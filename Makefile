CXX = g++
CXXFLAGS = -g -I../inc
TARGET = model
SRCS = src/model.cpp
OBJS = $(SRCS:.cpp=.o)
LIBS = -lm

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LIBS)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)
	rm -f $(TARGET)

plot:
	gnuplot plot/plot_attr_satis.gp
	gnuplot plot/plot_disc_satis.gp
	gnuplot plot/plot_male.gp
	gnuplot plot/plot_female.gp

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: plot plotm plotf run clean

