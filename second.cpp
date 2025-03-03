#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
//Запись имени файла
const std::string filename = "output.txt";
sem_t* file_sem;
//Поток записи
void* writerThread(void* arg) {
    //Указатель на разделяемую память
    char* data = static_cast<char*>(arg);
    //Подключение к уже существующему семафору
    sem_t* shm_sem = sem_open("/my_semaphore", 0);
    //Открытие файла в режиме добавления
    std::ofstream file(filename, std::ios::app);
    if (!file.is_open()) {
        std::cerr << "Ошибка открытия файла!" << std::endl;
        return nullptr;
    }
    while (true) {
        //Блокировка доступа разделяемой памяти
        sem_wait(shm_sem);
        //Чтение
        std::string str(data);
        //Разблокировка
        sem_post(shm_sem);
        if (!str.empty()) {
            //Блокировка
            sem_wait(file_sem);
            //Запись
            file << str << std::endl;
            //Принудительная запись
            file.flush();
            //Разблокировка
            sem_post(file_sem);
        }
        sleep(1);
    }
    //Закрытие семафора
    sem_close(shm_sem);
    return nullptr;
}
//Поток сортировки
void* sorterThread(void* arg) {
    while (true) {
        //Выполнение каждые 5 секунд
        sleep(5); 
        //Блокировка файла
        sem_wait(file_sem);
        //Чтение всех строк файла
        std::ifstream inFile(filename);
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(inFile, line)) {
            if (!line.empty()) lines.push_back(line);
        }
        inFile.close();
        //Сортировка
        std::sort(lines.begin(), lines.end());
        std::ofstream outFile(filename);
        for (const auto& l : lines) {
            outFile << l << std::endl;
        }
        outFile.close();
        //Разблокировка файла
        sem_post(file_sem);
    }
    return nullptr;
}
int main() {
    //Размер
    const int shmSize = 1024;
    //Ключ
    key_t key = 2025;
    //Создание сегмента, IPC_EXCL возвращает ошибку      
    file_sem = sem_open("/file_semaphore", O_CREAT, 0666, 1);
    if (file_sem == SEM_FAILED) {
        perror("sem_open(file)");
        return 1;
    }
    // Подключение к разделяемой памяти
    int shmid = shmget(key, shmSize, 0666);
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }
    // Подключение памяти
    char* data = static_cast<char*>(shmat(shmid, nullptr, 0));
    if (data == (char*)-1) {
        perror("shmat");
        return 1;
    }
    //Поток записи
    pthread_t writer_thread, sorter_thread;
    if (pthread_create(&writer_thread, nullptr, writerThread, data) != 0) {
        std::cerr << "Ошибка создания потока записи!" << std::endl;
        return 1;
    }
    //Поток сортировки
    if (pthread_create(&sorter_thread, nullptr, sorterThread, nullptr) != 0) {
        std::cerr << "Ошибка создания потока сортировки!" << std::endl;
        return 1;
    }
    //Ожидание заврешение потока (бесконечно)
    pthread_join(writer_thread, nullptr);
    pthread_join(sorter_thread, nullptr);
    //Освобождение ресурсов
    sem_close(file_sem);
    sem_unlink("/file_semaphore");
    shmdt(data);
    return 0;
}