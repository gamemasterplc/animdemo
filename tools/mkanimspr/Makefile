CXXFLAGS += -O3 -std=c++20
OBJDIR = build
SRCDIR = src

OBJ = $(OBJDIR)/main.o $(OBJDIR)/tinyxml2.o $(OBJDIR)/binwrite.o

all: mkanimspr

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

mkanimspr: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $ $(LINKFLAGS)

clean:
	rm -rf ./build ./mkanimspr

.PHONY: all clean