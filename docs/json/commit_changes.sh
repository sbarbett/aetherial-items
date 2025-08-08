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

# Sanity checks for aether.json
AETHER_FILE="docs/json/aether.json"

# 1. Check if file is empty
if [ ! -s "$AETHER_FILE" ]; then
    echo "Error: aether.json is empty"
    exit 1
fi

# 2. Check if jq is available
if ! command -v jq &> /dev/null; then
    echo "Error: jq is not installed. Please install jq to validate JSON."
    exit 1
fi

# 3. Validate JSON format
if ! jq empty "$AETHER_FILE" 2>/dev/null; then
    echo "Error: aether.json contains invalid JSON"
    exit 1
fi

# 4. Check if "objects" property exists and has at least one object
if ! jq -e '.objects | length > 0' "$AETHER_FILE" >/dev/null 2>&1; then
    echo "Error: aether.json must contain an 'objects' property with at least one object"
    exit 1
fi

echo "All sanity checks passed. Proceeding with commit..."

# Stage the changes
git add docs/json/aether.json

# Create commit with timestamp
TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
git commit -m "auto: update aether.json - $TIMESTAMP"

# Push the changes to the remote repository
git push

echo "Successfully committed changes to aether.json at $TIMESTAMP" 