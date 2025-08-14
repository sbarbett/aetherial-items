#!/usr/bin/env bash
# Script to commit docs/json/aether.json changes to git repository (cron-safe)

set -euo pipefail

# --- single-run lock (prevents overlapping cron runs) ---
LOCKFILE="/tmp/aetherial-items.commit.lock"
exec 9>"$LOCKFILE"
flock -n 9 || { echo "Another run is in progress; exiting."; exit 0; }
# (No need to delete $LOCKFILE; flock uses the FD)

export PATH="/usr/local/bin:/usr/bin:/bin"
export GIT_TERMINAL_PROMPT=0

# Resolve repo root (two levels up from this script)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$REPO_ROOT"

# Ensure we're in a git repo
git rev-parse --git-dir >/dev/null 2>&1 || {
  echo "Error: Not in a git repository at $REPO_ROOT"
  exit 1
}

AETHER_FILE="docs/json/aether.json"

# --- handle stale .git/index.lock safely ---
if [ -f .git/index.lock ]; then
  # If no active git process is touching this repo, consider removing an old lock
  if ! pgrep -f "git.*$(basename "$REPO_ROOT")" >/dev/null; then
    get_mtime() { stat -c %Y "$1" 2>/dev/null || stat -f %m "$1"; }
    now=$(date +%s)
    mtime=$(get_mtime .git/index.lock || echo "$now")
    age=$(( now - mtime ))
    if [ "$age" -gt 900 ]; then   # >15 minutes old → stale
      echo "Removing stale .git/index.lock (age ${age}s)"
      rm -f .git/index.lock
    else
      echo ".git/index.lock exists and is recent (${age}s); exiting to avoid corruption."
      exit 0
    fi
  else
    echo "Git process active in this repo; exiting to avoid conflict."
    exit 0
  fi
fi

# Optional: stay current with remote to avoid non-fast-forward on push
current_branch="$(git rev-parse --abbrev-ref HEAD)"
if [ "$current_branch" != "HEAD" ]; then
  git fetch --quiet origin || true
  if git rev-parse --verify "origin/$current_branch" >/dev/null 2>&1; then
    # Rebase local changes on top of remote
    if ! git rebase --autostash "origin/$current_branch"; then
      git rebase --abort || true
      echo "Rebase failed; exiting."
      exit 1
    fi
  fi
fi

# --- change detection (both working tree and index) ---
if git diff --quiet -- "$AETHER_FILE" && git diff --cached --quiet -- "$AETHER_FILE"; then
  echo "No changes detected in $AETHER_FILE"
  exit 0
fi

# --- sanity checks ---
# 1) File non-empty
[ -s "$AETHER_FILE" ] || { echo "Error: $AETHER_FILE is empty"; exit 1; }

# 2) jq present
command -v jq >/dev/null || { echo "Error: jq is not installed"; exit 1; }

# 3) Valid JSON
jq empty "$AETHER_FILE" >/dev/null 2>&1 || { echo "Error: invalid JSON in $AETHER_FILE"; exit 1; }

# 4) Must contain non-empty .objects
jq -e '.objects | length > 0' "$AETHER_FILE" >/dev/null 2>&1 || {
  echo "Error: $AETHER_FILE must contain '.objects' with at least one object"
  exit 1
}

echo "All sanity checks passed. Proceeding with commit..."

# --- stage, commit, push ---
git add -- "$AETHER_FILE"

TIMESTAMP="$(date '+%Y-%m-%d %H:%M:%S %z')"
if git commit -m "auto: update aether.json - $TIMESTAMP"; then
  git push --quiet
  echo "Successfully committed and pushed changes at $TIMESTAMP"
else
  echo "Nothing to commit (race with another run or identical content)."
fi
