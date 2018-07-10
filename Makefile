OUTPUT_OBJ_DIR = output
EXECUTABLE = log_p
CFLAGS = -c -g -Wall 
OBJECTS = main.o \
          file.o \
          ring_buffer.o \
          time_api.o \
          write_buff.o \
          log_level.o \
          log_inst.o \
          common_func.o \
          log_unit.o
OBJS = $(patsubst %,$(OUTPUT_OBJ_DIR)/%,$(OBJECTS))
RM = rm -rf
GDB = gdb
VGRND = valgrind
VGRNDFLAGS = --leak-check=full \
             --track-origins=yes \
			 --verbose \
             $(OUTPUT_OBJ_DIR)/$(EXECUTABLE)  

all : $(OBJS)
	cc -o $(OUTPUT_OBJ_DIR)/$(EXECUTABLE) -g $(OBJS)
	-@echo ' '

$(OUTPUT_OBJ_DIR)/main.o : main.c file.h sanity.h
	cc $(CFLAGS) -o $@ main.c

$(OUTPUT_OBJ_DIR)/file.o : file.c file.h sanity.h
	cc $(CFLAGS) -o $@ file.c

$(OUTPUT_OBJ_DIR)/ring_buffer.o : ring_buffer.c ring_buffer.h
	cc $(CFLAGS) -o $@ ring_buffer.c
	
$(OUTPUT_OBJ_DIR)/time_api.o : time_api.c time_api.h
	cc $(CFLAGS) -o $@ time_api.c
	
$(OUTPUT_OBJ_DIR)/write_buff.o : write_buff.c write_buff.h
	cc $(CFLAGS) -o $@ write_buff.c
	
$(OUTPUT_OBJ_DIR)/log_level.o : log_level.c log_level.h
	cc $(CFLAGS) -o $@ log_level.c
	
$(OUTPUT_OBJ_DIR)/log_inst.o : log_inst.c log_inst.h
	cc $(CFLAGS) -o $@ log_inst.c

$(OUTPUT_OBJ_DIR)/common_func.o : common_func.c common_func.h
	cc $(CFLAGS) -o $@ common_func.c

$(OUTPUT_OBJ_DIR)/log_unit.o : log_unit.c log_unit.h
	cc $(CFLAGS) -o $@ log_unit.c

clean :
	$(RM) $(OUTPUT_OBJ_DIR)/*.o $(OUTPUT_OBJ_DIR)/$(EXECUTABLE)
	-@echo ' '

run_dbg :
	$(GDB) $(OUTPUT_OBJ_DIR)/$(EXECUTABLE)

run_memchck:
	$(VGRND) $(VGRNDFLAGS)
