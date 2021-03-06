#/**----------------------------------------------------------------------------
# Tecnequip Tecnologia em Equipamentos Ltda                                 
#-------------------------------------------------------------------------------
# @author Alessandro Holanda
# @version 1.0
# @begin 06.02.2009
# @brief Script de linkagem dos modulos MiniLPC
#-----------------------------------------------------------------------------*/

AS=as
AR=ar
CC=gcc
CFLAGS=-g -Wall

TEST_FILE=TestDB

OBJ_APP   = $(patsubst %.c,%.o,$(wildcard *.c))
PROG_NAME = libDB

#------------------------------------------------------------------------------
# Implicit rule to compile all C files in the directory
#------------------------------------------------------------------------------

%.o : %.c 
	$(CC) $(CFLAGS) -c $< -o $@
	
#------------------------------------------------------------------------------
# default target: Generates library
#------------------------------------------------------------------------------
$(PROG_NAME):  makefile $(OBJ_APP)
	$(AR) -cvq $(PROG_NAME).a $(OBJ_APP)
	$(CC) -o $(TEST_FILE) $(TEST_FILE).o $(PROG_NAME).a -lmysqlclient -lsybdb

#------------------------------------------------------------------------------
# clean target: Removes all generated files
#------------------------------------------------------------------------------

clean:
	rm -rf *.o *.a
