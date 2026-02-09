SHELL := /usr/bin/env bash

.DEFAULT_GOAL := build

TARGET = dmb-timerd

CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -Iinclude/ -MMD -D'PROGRAM_NAME="$(TARGET)"'
LDFLAGS = -lcurl -lcjson -lpthread

BUILD_DIR = build/
SRC_DIR = src/
INIT_DIR = init/
SYSTEMD_DIR = $(INIT_DIR)systemd/

SRC_FILES = $(shell find $(SRC_DIR) -name '*.c')
OBJ_FILES = $(patsubst $(SRC_DIR)%, $(BUILD_DIR)%, $(SRC_FILES:.c=.o))

-include $(OBJ_FILES:.o=.d)

SYS_BIN_DIR = /usr/local/bin/
SYS_LOG_DIR = /var/log/$(TARGET)/
SYS_SYSTEMD_DIR = /etc/systemd/system/

define print_message
	echo -e "\033[0;33;1m$1...\033[0m"
endef

define print_success
	echo -e "\033[0;32;1m$1!\033[0m"
endef

define print_error
	echo -e "\033[0;31;1m$1!\033[0m"
endef

require_deps:
	@if ! which $(CC) &> /dev/null; then \
		$(call print_error,$(CC) not found); \
		exit 1; \
	fi

	@for flag in $(LDFLAGS); do \
		lib=lib$${flag#-l}.so; \
		if ! ldconfig -p | grep -Eq $$lib; then \
			$(call print_error,$$lib not found); \
			exit 1; \
		fi \
	done

require_root:
	@if [[ $$EUID -ne 0 ]]; then \
		$(call print_error,This target must be run as root); \
		exit 1; \
	fi

build: require_deps $(BUILD_DIR) $(BUILD_DIR)$(TARGET)

$(BUILD_DIR):
	@$(call print_message,Building $(TARGET))
	mkdir -p $(sort $(dir $(OBJ_FILES)))

$(BUILD_DIR)$(TARGET): $(OBJ_FILES)
	$(CC) $^ -o $@ $(LDFLAGS)
	@$(call print_success,Building $(TARGET) done)
	@$(call print_success,$$($(BUILD_DIR)$(TARGET) -v) on $$(uname -mo))

$(BUILD_DIR)%.o: $(SRC_DIR)%.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	@$(call print_message,Cleaning $(TARGET) build files)
	rm -rf $(BUILD_DIR)
	@$(call print_success,Cleaning $(TARGET) build files done)

install: require_root require_deps
	@$(call print_message,Installing $(TARGET))
	cp $(BUILD_DIR)$(TARGET) $(SYS_BIN_DIR)
	mkdir -p $(SYS_LOG_DIR)
	id $(TARGET) &> /dev/null || useradd -Mrs /bin/false $(TARGET)
	chown $(TARGET):$(TARGET) $(SYS_BIN_DIR)$(TARGET) $(SYS_LOG_DIR)
	chmod 770 $(SYS_BIN_DIR)$(TARGET) $(SYS_LOG_DIR)

	if which systemctl &> /dev/null; then \
		for file in $(wildcard $(SYSTEMD_DIR)*); do \
			sed -e 's/@TARGET@/$(TARGET)/g' -e 's|@SYS_BIN_DIR@|$(SYS_BIN_DIR)|g' "$$file" > \
			$(SYS_SYSTEMD_DIR)$$(basename $$file | sed 's/target/$(TARGET)/'); \
		done; \
		systemctl daemon-reload; \
	fi

	@$(call print_success,Installing $(TARGET) done)

uninstall: require_root
	@$(call print_message,Uninstalling $(TARGET))
	rm -rf $(SYS_BIN_DIR)$(TARGET) $(SYS_LOG_DIR)

	if which systemctl &> /dev/null; then \
		for file in $(wildcard $(SYSTEMD_DIR)*); do \
			rm -f $(SYS_SYSTEMD_DIR)$$(basename $$file | sed 's/target/$(TARGET)/'); \
		done; \
		systemctl daemon-reload; \
	fi

	if id $(TARGET) &> /dev/null; then \
		userdel $(TARGET); \
	fi

	@$(call print_success,Uninstalling $(TARGET) done)

.PHONY := build clean install uninstall
