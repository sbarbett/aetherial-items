CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
TARGET = area_to_json
SOURCE = area_to_json.c

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

clean:
	rm -f $(TARGET)

.PHONY: clean 