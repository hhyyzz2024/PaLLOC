CXX = g++
RM = rm -rf
CXXFLAGS = -std=c++14

PROJCT_DIR = $(shell pwd)
SRC_DIR = ./src
INC_DIR = ./inc
OBJ_DIR = ./obj

CXXFLAGS += -I$(INC_DIR) 

PCM_LIB_DIR = $(PROJCT_DIR)/third_party/pcm/build/lib
PCM_INC_DIR = $(PROJCT_DIR)/third_party/pcm/src

RDT_LIB_DIR = $(PROJCT_DIR)/third_party/intel-cmt-cat/lib
RDT_INC_DIR = $(PROJCT_DIR)/third_party/intel-cmt-cat/lib


CXXFLAGS += -I$(RDT_INC_DIR) -I$(PCM_INC_DIR)
LDFLAGS = -lrt -lpthread -lnuma -larmadillo -L$(RDT_LIB_DIR) -lpqos -L$(PCM_LIB_DIR) -lpcm -llikwid

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(notdir $(SRCS)))

ifeq ($(debug), 1)
#        CXXFLAGS += -D print_level=1 -DDEBUG_ON -g -O0
	CXXFLAGS += -g -O0
else
    CXXFLAGS += -O3
endif

ifeq ($(test), 1)
	CXXFLAGS += -DTEST
endif

ifeq ($(slef_test_ipc), 1)
	CXXFLAGS += -DTEST -DENABLE_SELF_MONITOR -DMONITORING_SELF_IPC
endif

ifeq ($(slef_test_llc), 1)
	CXXFLAGS += -DTEST -DENABLE_SELF_MONITOR -DMONITORING_SELF_LLC
endif

ifeq ($(slef_test_mb), 1)
	CXXFLAGS += -DTEST -DENABLE_SELF_MONITOR -DMONITORING_SELF_MB
endif

TARGET = PaLLOC

all: $(OBJ_DIR) $(TARGET)
$(TARGET): main.cpp $(OBJS) 
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp 
	$(CXX) -c $< -o $@ $(CXXFLAGS)

$(OBJ_DIR):
	test -d $(OBJ_DIR) || mkdir -p $(OBJ_DIR)

clean:
	$(RM) $(OBJS) $(TARGET)

.PHONY: all clean
