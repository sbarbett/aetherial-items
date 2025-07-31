FROM ubuntu:22.04

# Install necessary packages
RUN apt-get update && apt-get install -y \
    build-essential \
    cron \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Compile the C program
RUN make clean && make

# Create output directory
RUN mkdir -p /output

# Create a script to run the program
RUN echo '#!/bin/bash\n\
cd /app\n\
echo "Running area_to_json on $(date)"\n\
# Create timestamped error log filename\n\
ERROR_LOG="/output/error_$(date +%Y%m%d_%H%M%S).log"\n\
# Remove old error logs, keeping only the 2 most recent (plus the one being created)\n\
ls -t /output/error_*.log 2>/dev/null | tail -n +4 | xargs -r rm\n\
/app/area_to_json /area/aether.are > /output/aether.json 2> "$ERROR_LOG"\n\
echo "Completed at $(date)"' > /app/run_program.sh

# Make the script executable
RUN chmod +x /app/run_program.sh

# Set up cron job to run every 5 minutes
RUN echo "*/5 * * * * /app/run_program.sh" > /etc/cron.d/area_to_json

# Give execution rights on the cron job
RUN chmod 0644 /etc/cron.d/area_to_json

# Apply cron job
RUN crontab /etc/cron.d/area_to_json

# Create startup script
RUN echo '#!/bin/bash\n\
echo "Starting area_to_json container..."\n\
echo "Running initial execution..."\n\
/app/run_program.sh\n\
echo "Starting cron daemon..."\n\
cron -f' > /app/start.sh

# Make startup script executable
RUN chmod +x /app/start.sh

# Set the startup script as the entrypoint
ENTRYPOINT ["/app/start.sh"] 