
TARGET := ../GUI.dll

SRCEXT   := c
SRCDIR   := ./
INCPATH  := ../
OBJDIR   := .
BINDIR   := ./
BIN_DEBUGDIR := $(BINDIR)/debug
COMMON_DIR := ../../common
COMMON_SRC := dbg RawPacket PacketType crypto PacketProcessor zlib cwebsocket

INCLUDES := -I. \
			-I$(INCPATH) \
			-I$(COMMON_DIR)

# C compiler flag
CFLAGS := -Wno-unused-function -D_GNU_SOURCE -D__DBG_ACTIVATED__ -Wall -march=native $(INCLUDES) -c

# Linker library flags
LIBDIRS  := -L$(API_DIR)/lib
LDFLAGS  := -shared -Wl,-rpath -Wl,$(API_DIR)/lib $(LIBDIRS)
LDLIBS   := -lz -lws2_32

SRCS	:= $(shell find $(SRCDIR) -name '*.$(SRCEXT)')
COMMONS	:= $(shell find $(addprefix $(COMMON_DIR)/,$(COMMON_SRC)) -name '*.$(SRCEXT)')
SRCDIRS := $(shell find . -name '*.$(SRCEXT)' -exec dirname {} \; | uniq)
OBJS	:= $(patsubst %.$(SRCEXT),$(OBJDIR)/%.o,$(SRCS) $(COMMONS))

###########################################################################
# Rules to compile our files  - Do not change below this line!
###########################################################################
CFLAGS += -std=gnu99

.PHONY: all debug release clean distclean 

all: $(BINDIR)/$(TARGET)

# Build debug library
debug: CFLAGS += -g
debug: clean $(BIN_DEBUGDIR)/$(TARGET)

# strip all symbols from release verison
release: LDFLAGS += -s 
release: CFLAGS += -O2
release: $(BINDIR)/$(TARGET)

$(BINDIR)/$(TARGET): buildrepo $(OBJS)
	@mkdir -p `dirname $@`
	@echo "Linking $@..."
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)
	@echo "$@ sucessfully built."

$(BIN_DEBUGDIR)/$(TARGET): buildrepo $(OBJS)
	@mkdir -p `dirname $@`
	@echo "Linking $@..."
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)
	@echo "$@ sucessfully built."

$(OBJDIR)/%.o: %.$(SRCEXT)
	@echo "Generating dependencies for $<..."
	$(call make-depend,$<,$@,$(subst .o,.d,$@))
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $< -o $@

clean:
	@echo "Cleaning ..."
	$(RM) -rf $(TARGET) $(OBJS) $(subst .o,.d,$(OBJS))

distclean: clean
	$(RM) -r $(BINDIR)/$(TARGET)
	$(RM) -r $(BIN_DEBUGDIR)/$(TARGET)

buildrepo:
	$(call make-repo)

define make-repo
   for dir in $(SRCDIRS); \
   do \
	mkdir -p $(OBJDIR)/$$dir; \
   done
endef

# usage: $(call make-depend,source-file,object-file,depend-file)
define make-depend
  $(CC) -MM	   \
		-MMD	  \
		-MF $3	\
		-MP	   \
		-MT $2	\
		$(CFLAGS) \
		$(LDFLAGS) \
		$1
endef