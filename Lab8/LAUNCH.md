# Сервер на C

Этот проект представляет собой сервер и клиента, написанный на языке программирования C.

## Компиляция

Чтобы скомпилировать сервер и клиента, выполните следующие команды в терминале:

```bash
gcc -o server server.c
gcc -o client client.c
```

## Запуск

Чтобы запустить сервер, выполните следующую команду в терминале:
```bash
./server <port> <base_directory>
```
где:

- `<port>` - это порт, на котором будет работать сервер.
- `<base_directory>` - это базовый каталог, который будет использоваться сервером.

Чтобы запустить клиент, выполните следующую команду в терминале:
```bash
./client <hostname> <port>
```
где:

- `<hostname>` - это имя хоста, на котором работает сервер.
- `<port>` - это порт, на котором работает сервер.

## Команды

Сервер поддерживает следующие команды:

- `ECHO <message>`: сервер повторяет полученное сообщение.
- `INFO`: сервер возвращает информацию из файла text.txt.
- `QUIT`: завершает соединение с сервером.
- `LIST`: сервер возвращает список файлов в текущем каталоге.
- `CD <directory>`: сервер меняет текущий каталог на указанный.