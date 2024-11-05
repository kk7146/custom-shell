SRCS	= srcs/customshell.c srcs/commandfunc.c
OBJS	= $(SRCS:.c=.o)
NAME	= custom-shell
INCL	= includes
FLAG	= -Wall -Werror -Wextra
CC		= gcc

all: $(NAME)

.c.o :
	$(CC) -c $< -o $(<:.c=.o) -I $(INCL)

$(NAME) : $(OBJS)
	$(CC) $(FLAG) -o $(NAME) $(OBJS)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all