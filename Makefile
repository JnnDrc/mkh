CC := gcc
CFLAGS := -Wall -Wextra -O3

mkh: mkh.c
	$(CC) $^ -o $@ $(CFLAGS)
