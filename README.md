# aetherial-items

A toolchain for parsing and displaying legendary ("aetherial") items from Acropolis in an interactive web interface.

## Overview

This project consists of two main components:

1. **Parser Container** (`src/`) - A Docker container that reads MUD area file and extracts aetherial items into structured JSON
2. **Web Viewer** (`app/`) - An interactive web application that displays the parsed items as cards

## Quick Start

### 1. Parse Area Files
```bash
cd src
echo "AREA_FILE_PATH=/path/to/your/area/folder" > .env
docker-compose up -d
```

### 2. View Items
```bash
cd app
python3 -m http.server 8000
# Open http://localhost:8000
```

## Project Structure

```
├── src/                    # Area file parser (Docker container)
│   ├── area_to_json.c      # C program that parses area files
│   ├── docker-compose.yml  # Container orchestration
│   └── README.md           # Parser documentation
└── app/                    # Web viewer application
    ├── index.html          # Main web interface
    ├── script.js           # Interactive functionality
    ├── styles.css          # Theming
    ├── README.md           # Web app documentation
    └── json/               # Generated item data (created by parser)
        └── aether.json     # Structured item data
```
## Documentation

- [Parser Documentation](src/README.md) - Details on the area file parser
- [Web App Documentation](app/README.md) - Details on the web interface 