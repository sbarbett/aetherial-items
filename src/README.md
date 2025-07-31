# Area to JSON Docker Container

Converts MUD area file objects to JSON format using a C program that runs every 5 minutes.

## Quick Start

1. **Create output directory:**
   ```bash
   mkdir -p ../json
   ```

2. **Set up environment:**
   ```bash
   cd src
   echo "AREA_FILE_PATH=/path/to/your/area/folder" > .env
   ```

3. **Run container:**
   ```bash
   docker-compose up -d
   ```

## Configuration

- **Input:** `/area/somearea.are` (mounted from host)
- **Output:** `../app/json`
- **Schedule:** Every 5 minutes via cron
- **Error logs:** Up to 3 timestamped files kept

## Commands

```bash
# View logs
docker-compose logs -f

# Stop container
docker-compose down

# Rebuild after code changes
docker-compose build --no-cache && docker-compose up -d
``` 