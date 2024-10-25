#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <unistd.h>

int buffer_size;
int num_consumer_threads;

pthread_t *thread_pool;
int *thread_availability;
pthread_t main_thread;

pthread_mutex_t buffer_mutex;
pthread_mutex_t output_mutex;

typedef struct {
    char *sourceDirectory;
    char *destDirectory;
    int avaibleThread;
} Files;

void err_sys(const char *x) {
    perror(x);
    exit(-1);
}

double get_elapsed_time(struct timeval start, struct timeval end) {
    return (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_usec - start.tv_usec) / 1000000.0;
}
void ignore_sigint(int sig) {
    printf("....Pressed ctrl-c\n");
    for (int i = 0; i < num_consumer_threads; ++i)
    {
        pthread_cancel(thread_pool[i]);
    }
    free(thread_pool);
    free(thread_availability);
    exit(EXIT_SUCCESS);
}
int delete_directory(const char *path) {
    DIR *dir;
    struct dirent *entry;
    char entry_path[257];

    dir = opendir(path);
    if (dir == NULL) {
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            // Geçerli dizin girişi "." veya ".." ise atla
            continue;
        }

        // Geçerli dizin girişinin yolu
        snprintf(entry_path, sizeof(entry_path), "%s/%s", path, entry->d_name);

        if (entry->d_type == DT_DIR) {
            // Dizinse, içeriğini sil
            delete_directory(entry_path);
        } else {
            // Dosyaysa, sil
            remove(entry_path);
        }
    }

    closedir(dir);

    // Klasörü sil
    if (rmdir(path) != 0) {
        return -1;
    }

    return 0;
}

void customer(const char *source, const char *dest) {
    FILE *source_file, *dest_file;
    char karakter;

    source_file = fopen(source, "rb");
    dest_file = fopen(dest, "wb");

    if (source_file == NULL || dest_file == NULL) {
        err_sys("Could not open directory\n");
    }

    while ((karakter = fgetc(source_file)) != EOF) {
        fputc(karakter, dest_file);
    }

    fclose(source_file);
    fclose(dest_file);
}

void *customer_thread(void *arg) {
    Files *files = (Files *)arg;
    pthread_mutex_lock(&output_mutex);
    customer(files->sourceDirectory, files->destDirectory);
    pthread_mutex_unlock(&output_mutex);

    pthread_mutex_lock(&buffer_mutex);
    thread_availability[files->avaibleThread] = 0;
    pthread_mutex_unlock(&buffer_mutex);

    return NULL;
}

void producer(const char *source_dizin, const char *dest_dizin) {
    DIR *dir;
    struct dirent *dosya;
    struct stat stat_info;

    dir = opendir(source_dizin);

    if (dir == NULL) {
        err_sys("Could not open directory.\n");
    }

    while ((dosya = readdir(dir)) != NULL) {
        if (strcmp(dosya->d_name, ".") != 0 && strcmp(dosya->d_name, "..") != 0) {

            char source_yol[257];
            char dest_yol[257];

            snprintf(source_yol, sizeof(source_yol), "%s/%s", source_dizin, dosya->d_name);
            snprintf(dest_yol, sizeof(dest_yol), "%s/%s", dest_dizin, dosya->d_name);

            if (stat(source_yol, &stat_info) == 0) {
                if (S_ISDIR(stat_info.st_mode)) {
                    printf("Copied Folder: %s\n",dest_yol);
                    mkdir(dest_yol, 0700);
                    producer(source_yol, dest_yol);
                } else if (S_ISREG(stat_info.st_mode)) {

                    while (1) {
                        int available_thread = -1;
                        pthread_mutex_lock(&buffer_mutex);
                        for (int i = 0; i < num_consumer_threads; ++i) {
                            if (thread_availability[i] == 0) {
                                available_thread = i;
                                break;
                            }
                        }
                        pthread_mutex_unlock(&buffer_mutex);

                        if (available_thread != -1) {
                            pthread_mutex_lock(&buffer_mutex);
                            thread_availability[available_thread] = 1;
                            pthread_mutex_unlock(&buffer_mutex);

                            Files filesCustomer;
                            filesCustomer.sourceDirectory = source_yol;
                            filesCustomer.destDirectory = dest_yol;
                            filesCustomer.avaibleThread = available_thread;

                            pthread_mutex_lock(&buffer_mutex);
                            pthread_create(&thread_pool[available_thread], NULL, customer_thread, &filesCustomer);
                            pthread_mutex_unlock(&buffer_mutex);
                            printf("Copied File: %s\n",dest_yol);
                            pthread_join(thread_pool[available_thread], NULL);
                            break;
                        }
                    }
                }else if (S_ISFIFO(stat_info.st_mode)) {
                    printf("Copied Fifo: %s\n",dest_yol);
                    mkfifo(dest_yol, 0666);
                }

            }
        }
    }

    closedir(dir);
}

void *producer_thread(void *arg) {
    Files *files = (Files *)arg;
    producer(files->sourceDirectory, files->destDirectory);

    for (int i = 0; i < num_consumer_threads; ++i) {
        pthread_join(thread_pool[i], NULL);
    }

    return NULL;
}

int main(int argc, char *argv[]) {

    struct sigaction sa;
    sa.sa_handler = ignore_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    if (argc != 5) {
        err_sys("Usage: ./pCp <buffer_size> <num_consumer_threads> <source_directory> <destination_directory>\n");
    }

    buffer_size = atoi(argv[1]);
    num_consumer_threads = atoi(argv[2]);
    Files files;
    files.destDirectory = argv[4];
    files.sourceDirectory = argv[3];

    thread_availability = (int *)malloc(num_consumer_threads * sizeof(int));
    thread_pool = (pthread_t *)malloc(num_consumer_threads * sizeof(pthread_t));

    if (buffer_size <= 0 || num_consumer_threads <= 0) {
        printf("Buffer size and number of consumer threads must be positive integers\n");
        return 1;
    }

    /******thread pool**/

    for (int i = 0; i < num_consumer_threads; ++i) {
        thread_availability[i] = 0; // 0 ise müsait 1 ise değil
    }

    double elapsed_time;
    struct timeval start_time, end_time;

    delete_directory(files.destDirectory);

    mkdir(files.destDirectory, 0700);
    printf("\nCopying from %s folder to %s folder.\n",files.sourceDirectory,files.destDirectory);
    gettimeofday(&start_time, NULL);
    pthread_mutex_init(&buffer_mutex, NULL);
    pthread_mutex_init(&output_mutex, NULL);

    pthread_create(&main_thread, NULL, producer_thread, &files);

    pthread_join(main_thread, NULL);

    pthread_mutex_destroy(&buffer_mutex);
    pthread_mutex_destroy(&output_mutex);

    gettimeofday(&end_time, NULL);

    elapsed_time = get_elapsed_time(start_time, end_time);

    printf("Elapsed time: %.6f seconds\n", elapsed_time);
    printf("Finished..\n");

    free(thread_availability);
    free(thread_pool);

    return 0;
}