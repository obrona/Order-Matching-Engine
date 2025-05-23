CC = gcc
CXX = g++

CFLAGS := $(CFLAGS) -g -O3 -Wall -Wextra -pedantic -Werror -std=c18 -pthread
CXXFLAGS := $(CXXFLAGS) -g -O3 -Wall -Wextra -pedantic -Werror -std=c++20 -pthread

BUILDDIR = build

SRCS = main.cpp engine.cpp io.cpp

all: engine client

engine: $(SRCS:%=$(BUILDDIR)/%.o)
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

client: $(BUILDDIR)/client.cpp.o
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

debug_engine: 
	clang++ -std=c++20 main.cpp engine.cpp io.cpp -g -o debug_engine

engine2:
	clang++ -std=c++20 -latomic  -fsanitize=thread -O2 -g main.cpp engine.cpp io.cpp -o engine

engine3:
	clang++ -std=c++20 -latomic  -fsanitize=address -O2 -g main.cpp engine.cpp io.cpp -o engine

.PHONY: clean
clean:
	rm -rf $(BUILDDIR)
	rm -f client engine

DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILDDIR)/$<.d
COMPILE.cpp = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c

$(BUILDDIR)/%.cpp.o: %.cpp | $(BUILDDIR)
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<

$(BUILDDIR): ; @mkdir -p $@

DEPFILES := $(SRCS:%=$(BUILDDIR)/%.d) $(BUILDDIR)/client.cpp.d

-include $(DEPFILES)
