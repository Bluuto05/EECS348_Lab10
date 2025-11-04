TARGET = str_calc
SRC    = main.cpp

$(TARGET): $(SRC)
	$(CXX) -std=c++17 -O2 -Wall -Wextra -o $(TARGET) $(SRC)

.PHONY: clean
clean:
	rm -f $(TARGET)
