SRC_DIR := src
OBJ_DIR := obj
OUT ?= build

EXE := $(BIN_DIR)/main
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

CPPFLAGS := -Iinclude -MMD -MP
CFLAGS   := -Iinclude  -Wall -g
LDFLAGS  := -Llib
LDLIBS   := -lm

.PHONY: all clean distclean

SHELL_HACK := $(shell mkdir -p $(OUT))

EXEC = $(OUT)/main
all: $(EXEC)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OUT)/main: $(OBJ)
	$(CC) $(CFLAGS) -o $@ tests/main.c $^

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

clean:
	@$(RM) -rv $(OBJ_DIR)
distclean: clean
	$(RM) -r $(OUT)