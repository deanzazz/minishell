CC = gcc
TARGET = minishell

$(TARGET): minishell.c
	$(CC) -o $(TARGET) minishell.c

clean:
	rm -f $(TARGET)
