#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(dirname "$SCRIPT_DIR")"
VENV_DIR="$WORKSPACE_DIR/.venv"
SCRIPT_DIR_NAME="$(basename "$SCRIPT_DIR")"

# Check required system dependencies
missing=()

for cmd in python3 cmake dtc; do
    if ! command -v "$cmd" &>/dev/null; then
        missing+=("$cmd")
    fi
done

if [ ${#missing[@]} -ne 0 ]; then
    echo "Error: missing required dependencies: ${missing[*]}"
    echo ""
    echo "Install them before running this script. For example:"
    echo "  sudo apt install python3 python3-venv cmake device-tree-compiler  # Debian/Ubuntu"
    echo "  sudo pacman -S python3 cmake dtc                                  # Arch"
    exit 1
fi

# Warn if workspace directory contains unexpected contents
other_files="$(ls -A "$WORKSPACE_DIR" | grep -v "^${SCRIPT_DIR_NAME}$" | grep -v '^\.\(venv\|west\)$')"
if [ -n "$other_files" ]; then
    echo "Warning: workspace directory $WORKSPACE_DIR contains unexpected files:"
    echo "$other_files"
    read -rp "Continue anyway? [y/N] " answer
    if [[ ! "$answer" =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Create venv and install west if needed
if [ ! -d "$VENV_DIR" ]; then
    echo "Creating Python virtual environment..."
    python3 -m venv "$VENV_DIR"
fi

source "$VENV_DIR/bin/activate"

if ! command -v west &>/dev/null; then
    echo "Installing west..."
    pip install west
fi

# Initialize west workspace with this repo as the manifest
if [ ! -d "$WORKSPACE_DIR/.west" ]; then
    echo "Initializing west workspace..."
    west init -l "$SCRIPT_DIR" --mf west.yml
fi

# Update west modules
echo "Updating west modules..."
cd "$WORKSPACE_DIR"
west update

echo "Workspace setup complete."
echo "Activate the venv with: source $VENV_DIR/bin/activate"
