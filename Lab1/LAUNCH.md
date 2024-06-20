# Программа для обхода директорий

Эта программа обходит директории и выводит информацию о файлах, символических ссылках и поддиректориях.

## Компиляция

Для компиляции программы используйте gcc:

```bash
gcc -o dirwalk dirwalk.c
```

## Запуск

Вы можете запустить программу, используя следующую команду:

```bash
./dirwalk
```

Программа принимает следующие опции:

- `-f`: Выводить информацию о файлах.
- `-l`: Выводить информацию о символических ссылках.
- `-d`: Выводить информацию о директориях.
- `-s`: Сортировать вывод.

Если не указаны опции `-f`, `-l` или `-d`, программа будет выводить информацию о всех трех типах объектов.

Вы также можете указать директорию для обхода:

```bash
./dirwalk /path/to/directory
```

Пожалуйста, замените /path/to/directory на путь к директории, которую вы хотите обойти.
Если директория не указана, программа будет обходить текущую директорию.