INC_DIR=./inc
OBJ_DIR=./obj
LIB_DIR=./lib
SRC_DIR=./src
BIN_DIR=./bin

CC=gcc
RM=rm -f

BOOTSTRAPFILES=/bootstrapfiles

TARGET=app.out


CFLAGS=-I${INC_DIR} -I${INC_DIR}/${BOOTSTRAPFILES}

${OBJ_DIR}/%.o : ${SRC_DIR}/%.c ${INC_DIR}/%.h
	${CC} ${CFLAGS} -c $< -o $@

${BIN_DIR}/$(TARGET) : ${OBJ_DIR}/${BOOTSTRAPFILES}/*.o
	gcc -o $@ ${OBJ_DIR}/*.o $(OBJ_DIR)/${BOOTSTRAPFILES}/*.o

route :
	route.py routing.con routing1.con

all : route ${BIN_DIR}/$(TARGET)

run : route all

clean :
	$(RM) $(BIN_DIR)/*.out $(OBJ_DIR)/*.o $(OBJ_DIR)/${BOOTSTRAPFILES}/*.o