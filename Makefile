#Author:cinience@gmail.com
#edit "src" to your path,it include your c source file
SRC_DIR =.

#edit "inc" to your path,it include your c header file
INC_DIR =.

#edit your .o file
OBJECTS01 = ftpclient.o
OBJECTS02 = chklogin.o clientinfo.o ftpserver.o

#edit your app name
TARGETS01 = client
TARGETS02 = server

#edit your complie
cc = gcc

#edit 
CFLAGS = -Wall -O -g

#VPATH = src:inc
vpath %.c $(SRC_DIR)
vpath %.h $(INC_DIR)

all : $(TARGETS01)  $(TARGETS02) 

#edit "myapp" to your app name
$(TARGETS01) : $(OBJECTS01)
	$(cc) $^ $(CFLAGS) -o $@ 
$(OBJECTS) : %.o : %.c
	$(cc) $(CFLAGS) -c $< -I$(INC_DIR)
	

$(TARGETS02) : $(OBJECTS02)
	$(cc)  $^ $(CFLAGS) -o $@ -lphread -lmysqlclient
$(OBJECTS) : %.o : %.c
	$(cc) $(CFLAGS) -c $< -I$(INC_DIR) 

.PHONY : clean
clean :
	-rm -f *.o $(TARGETS01) $(TARGETS02)

