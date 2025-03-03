#include <iostream>
#include <string>
#include <cstring>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <semaphore.h>
int main() {
    //Размер
    const int shmSize = 1024;
    //Ключ
    key_t key = 2025;
    //Создание сегмента, IPC_EXCL возвращает ошибку      
    int shmid = shmget(key, shmSize, IPC_CREAT | 0666);
    //Пролёт в создании
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }
    //Его подключение
    char* data = static_cast<char*>(shmat(shmid, nullptr, 0));
    //Ошибка подключения
    if (data == (char*)-1) {
        perror("shmat");
        return 1;
    }
    //Создание семафора (имя, флаг, права доступа, разблокировка)
    sem_t* sem = sem_open("/my_semaphore", O_CREAT, 0666, 1);
    //Проверка открытия семафора
    if (sem == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }
    /*Бесконечный цикл ввода значений,
      до тех пора пока не будет ввода "exit"*/
    std::string input;
    while (true) {
        std::cout << "Введите строку или \"exit\" для выхода: ";
        std::getline(std::cin, input);
        if (input == "exit") break;
        sem_wait(sem);
        std::strcpy(data, input.c_str());
        sem_post(sem);
    }
    //Закрытие семафора
    sem_close(sem);
    //Удаление имени семафора
    sem_unlink("/my_semaphore");
    //Откл. сегмента
    shmdt(data);
    //Удаления сегмента
    shmctl(shmid, IPC_RMID, nullptr);
    return 0;
}