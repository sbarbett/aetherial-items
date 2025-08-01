#!/bin/bash

# Script to commit aether.json changes to git repository

# Get the absolute path to the script directory
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# Get the repository root (two levels up from json/)
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Change to the repository root directory
cd "$REPO_ROOT"

# Check if we're in a git repository
if ! git rev-parse --git-dir > /dev/null 2>&1; then
    echo "Error: Not in a git repository at $REPO_ROOT"
    exit 1
fi

# Check if aether.json has been modified
if git diff --quiet docs/json/aether.json; then
    echo "No changes detected in aether.json"
    exit 0
fi

# Stage the changes
git add docs/json/aether.json

# Create commit with timestamp
TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
git commit -m "auto: update aether.json - $TIMESTAMP"

# Push the changes to the remote repository
git push

echo "Successfully committed changes to aether.json at $TIMESTAMP" 