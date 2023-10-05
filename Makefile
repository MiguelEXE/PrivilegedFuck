SOURCE_FILE = main.c
ifndef TEST_FILE
TEST_FILE = tests/fork.bf
endif
ifndef VERBOSE
.SILENT:
endif
all: compile
compile:
	mkdir -p out
	gcc $(SOURCE_FILE) -o out/pf -static
	echo "Compiled to out/pf"
test: binary_exists_test
	echo Testing...
	./out/pf -p -f $(TEST_FILE)
install: binary_exists_install needs_root
	cp out/pf /usr/bin/pf
	echo "PrivilegedFuck is now installed. Run 'pf' to see more information"
uninstall: needs_root
	rm -f /usr/bin/pf
	echo "PrivilegedFuck is now uninstalled."

binary_exists_install:
	@test -e out/pf || { echo "Please run 'make compile' before installing." ; exit 1; }
binary_exists_test:
	@test -e out/pf || { echo "Please run 'make compile' before testing." ; exit 1; }
needs_root:
	@if [ $$(id -u) -ne 0 ]; then \
		echo "Please run this command again with root privilegies."; \
		exit 1; \
	fi