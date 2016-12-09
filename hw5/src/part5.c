#include "lott.h"
#include "lott2.h"
#include <sys/socket.h>
#include <sys/poll.h>

static void* map(void*);
static void* reduce(void*);
static char* name(char*, int, int);
static int nfiles(char*);

int map_flag, limit, nthreads2;
int getRunning = 1;
int* rec;

int part5(size_t nthreads) {
    File_stats* head = NULL;
    head = calloc(1,sizeof(File_stats));
    head->duration = -2;
    int naming_number = 0;
    nthreads2 = nthreads;

    DIR* ptr;
    struct dirent *someptr;

    if(nthreads == 0)
        return -1;

    int number_files = nfiles(DATA_DIR);
    limit = number_files;
    if(number_files == 0)
        return -1;
    int files_per = number_files / nthreads;
    int remainder = number_files % nthreads;

    if(number_files <= (int) nthreads){
        files_per = 1;
        nthreads = number_files;
        remainder = 0;
    }
    
    int files_array[nthreads];
    for(int i = 0; i < nthreads; i++)
        files_array[i] = files_per;

    for(int i = 0; i < nthreads; i++){
        if(remainder > 0){
            files_array[i]++;
            remainder--;
        }
        if(remainder == 0)
            break;
    }

    if((ptr = opendir(DATA_DIR)) == NULL)
        return -1;

    File_stats* current = head;

    while((someptr = readdir(ptr)) != NULL){ //this will go through each individual file
        if(strcmp(someptr->d_name, "..") == 0)
            continue;
        else if (strcmp(someptr->d_name, ".") == 0)
            continue;
        else if(someptr->d_type == DT_REG){
            current->filename = calloc(256,sizeof(char));
            strcpy(current->filename,someptr->d_name);
            current->files = 0;

            long saved = telldir(ptr);
            if((someptr = readdir(ptr)) != NULL){
                if(strcmp(someptr->d_name, ".") != 0 || strcmp(someptr->d_name, "..") != 0){
                    current->next = calloc(1, sizeof(File_stats));
                    current = current->next;
                }
            }
            seekdir(ptr, saved);
        }
    }
    closedir(ptr);

    current = head;
    int index = 0;
    int left = 0;
    int heat_count = 0;
    File_stats* thread_heads[nthreads];
    int dead[nthreads][2];
    int deady = 0;
    rec = calloc(nthreads, sizeof(int));
    while(current != NULL){
        if(left == 0 && index <= nthreads){
            left = files_array[index];
            current->files = left;
            thread_heads[heat_count] = current;

            //WE WANT A SOCKET BETWEEN THE MAP AND REDUCE THREADS
            //THE FILE DESCRIPTOR FOR THE MAP IS IN THE STRUCT
            //THE OTHER DESCRIPTOR (RECEIVER) IS IN A GLOBAL RECIEVE ARRAY OF NTHREADS SIZE
            socketpair(AF_UNIX, SOCK_STREAM, 0, dead[deady]);
            deady++;
            current->fd = dead[deady - 1][0];
            rec[deady - 1] = dead[deady - 1][1];

            left--;
            index++;
            heat_count++;
        }else if(left > 0)
            left--;

        current = current->next;
    }

    current = head;

    for(int i = 0; i < nthreads; i++){
        char* name_now = calloc(128, sizeof(char));
        name_now = name(name_now, 0, naming_number);
        pthread_create(&thread_heads[i]->tid, NULL, map, thread_heads[i]);
        pthread_setname_np(thread_heads[i]->tid, name_now);
        naming_number++;
    }

    Reduce_stats* final = calloc(1, sizeof(Reduce_stats));
    pthread_create(&final->tid, NULL, reduce, final);
    char* name_now = calloc(128, sizeof(char));
    name_now = name(name_now, 1, 0);
    pthread_setname_np(final->tid, name_now);
    free(name_now);

    current = head;
    while(current != NULL){
        pthread_t x = current->tid;
        pthread_join(x, NULL);
        current = current->next;
        map_flag++;
    }
    if(map_flag == number_files){
        getRunning = 0;
    }
    pthread_cancel(final->tid);
    pthread_join(final->tid, NULL);
    printf(
        "Part: %s\n"
        "Query: %s\n",
        PART_STRINGS[current_part], QUERY_STRINGS[current_query]);

    if(strcmp(QUERY_STRINGS[current_query], "A") == 0)
        printf("Result: %.5g, %s\n", final->max_durr, final->max_file);
    else if(strcmp(QUERY_STRINGS[current_query], "B") == 0)
        printf("Result: %.5g, %s\n", final->min_durr, final->min_file);
    else if(strcmp(QUERY_STRINGS[current_query], "C") == 0)
        printf("Result: %.5g, %s\n", final->max_users, final->max_file);
    else if(strcmp(QUERY_STRINGS[current_query], "D") == 0)
        printf("Result: %.5g, %s\n", final->min_users, final->min_file);
    else if((strcmp(QUERY_STRINGS[current_query], "E") == 0))
        printf("Result: %.5g, %s\n", final->country_max, final->country);

    return 0;
}

static void* map(void* v){
    //printf("Hello from: %lu\n", pthread_self());
    File_stats* abc = (File_stats*) v;
    time_t now;
    struct tm ts;
    int holy = abc->files;
    int heady = abc->fd;

    for(int a = 0; a < holy; a++){
        char* filename_replace = calloc(1024, sizeof(char));

        strcat(filename_replace, abc->filename);
        abc->filename_t = filename_replace;

        char* country_index[10];
        int country_counter[10];
        for(int i = 0; i < 10; i++){
            country_index[i] = NULL;
            country_counter[i] = 0;
        }
        int year_check[138];
        for(int i = 0; i < 138; i++){
            year_check[i] = 0;
        }

        char* opening = calloc(1024, sizeof(char));
        strcat(opening, DATA_DIR);
        strcat(opening, "/");

        strcat(opening, abc->filename);
        //printf("Printing file name: %s\n", opening);

        FILE* current_file;
        current_file = fopen(opening, "r");

        //parse the text into its individual stats
        int user_counter = 0;
        int year = 0;
        char* year_string = calloc(1024, sizeof(char));
        int duration = 0;

        char* total_string = calloc(1024, sizeof(char));
        char* unix_string = calloc(1024, sizeof(char));
        char* dur_string = calloc(1024, sizeof(char));
        char* country = calloc(1024, sizeof(char));
        char* rest;

        while(fscanf(current_file, "%s", total_string) != EOF){
            int c_duration = 0;
            unix_string = strtok_r(total_string, ",", &rest);
            strtok_r(NULL, ",", &rest);
            dur_string = strtok_r(NULL, ",", &rest);
            country = strtok_r(NULL, ",", &rest);

            user_counter++; //raise the counter of users
            c_duration = atoi(dur_string);
            duration = duration + c_duration; //raise the total duration

            now = (time_t) atoi(unix_string); //change the UNIX to time_n
            localtime_r(&now, &ts);

            strftime(year_string, sizeof(year_string), "%Y", &ts);
            //printf("Year %s\n", year_string);
            year = atoi(year_string);
            year = year - 1901;
            year_check[year]++; //for nonzero years

            int index = 0;
            int fit = 0;

            while(country_index[index] != NULL){
                if(strcmp(country, country_index[index]) == 0){
                    country_counter[index]++;
                    fit = 1;
                    break;
                }
                index++;
            }
            if(fit == 0){
                country_index[index] = strdup(country);
                country_counter[index]++;
            }
        }

        double avg_duration = ((double)duration)/ user_counter;
        abc->duration = avg_duration;

        int nonzero_years = 0;
        for(int i = 0; i < 138; i++){
            if(year_check[i] != 0)
                nonzero_years++;
        }
        abc->user_count = user_counter;
        abc->nonzero_years = nonzero_years;
        abc->avg_usercount = ((double) user_counter) / nonzero_years;

        int max = 0;
        int max_index = 0;
        for(int i = 0; i < 10; i++){
            if(max < country_counter[i]){ //the first one will be the max
                max = country_counter[i];
                max_index = i;
            }
        }

        for(int i = 0; i < 10; i++){
            if(country_counter[i] == max){
                if(strcmp(country_index[max_index], country_index[i]) > 0)
                    max_index = i;
            }
        }

        abc->country = country_index[max_index];
        abc->country_counter = country_counter[max_index];
        //WRITE INTO THE SOCKET
        char* t = calloc(1024, sizeof(char));
        char* x = calloc(128, sizeof(char));

        strcat(t, abc->filename_t);
        strcat(t, ",");
        sprintf(x, "%f", abc->duration);
        strcat(t, x); //change avg_duration to a string
        strcat(t, ",");
        sprintf(x, "%f", abc->avg_usercount);
        strcat(t, x); //change avg_usercount to a string
        strcat(t, ",");
        strcat(t, abc->country);
        strcat(t, ",");
        sprintf(x, "%d", abc->country_counter);
        strcat(t, x);
        //WRITE THE STRING INTO THE SOCKET
        write(heady, t, 1024);

        fclose(current_file);
        free(x);
        free(t);

        abc = abc->next;

    }
    return NULL; //TO MAKE SURE THAT REDUCE IS USING THE FILE
}

static void* reduce(void* v){
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    File_stats* current = calloc(1, sizeof(File_stats));
    Reduce_stats* final = (Reduce_stats*) v;
    char* max_file = calloc(1024, sizeof(char));
    char* min_file = calloc(1024, sizeof(char));
    double max = 0;
    double min = 999999999;
    char** tcountry_index = calloc(1024, sizeof(char*));
    int* tcountry_counter = calloc(1024, sizeof(int));
    char* current_country = calloc(1024, sizeof(char));
    char* total_string = calloc(1024, sizeof(char));
    char* rest;

    while(1){
        //SETTING UP THE POLL TO RECIEVE FROM THE STRING FROM THE SOCKET
        struct pollfd polar[nthreads2];
        for(int i = 0; i < nthreads2; i++){
            polar[i].fd = rec[i];
            polar[i].events = POLLIN;
        }
        int getting_answer = poll(polar, nthreads2, -1);
        if(getting_answer < 0)
            break;
        if(getting_answer >= 0){ //READ FROM THE SOCKET
            for(int i = 0; i < nthreads2; i++){
                if(polar[i].revents & POLLIN){ 
                    read(polar[i].fd, total_string, 1024);

                    current->filename_t = strdup(strtok_r(total_string, ",", &rest));
                    current->duration = atof(strtok_r(NULL, ",", &rest));
                    current->avg_usercount = atof(strtok_r(NULL, ",", &rest));
                    current->country = strdup(strtok_r(NULL, ",", &rest));
                    current->country_counter = atoi(strtok_r(NULL, ",", &rest));

                    if(strcmp(QUERY_STRINGS[current_query], "A") == 0 || strcmp(QUERY_STRINGS[current_query], "B") == 0){
                        //FILENAME OF THE MAX AND MIN FILES NEEED TO BE DETERMINED
                        if(max < current->duration){
                            max = current->duration;
                            max_file = current->filename_t;
                        }
                        else if(max == current->duration){
                            if(strcmp(max_file, current->filename_t) > 0)
                                max_file = current->filename_t;
                        }

                        if(min > current->duration){
                            min = current->duration;
                            min_file = current->filename_t;
                        }
                        else if(min == current->duration){
                            if(strcmp(min_file, current->filename_t) > 0)
                                min_file = current->filename_t;
                        }

                        final->max_durr = max; //A
                        final->min_durr = min; //B
                        final->max_file = max_file;
                        final->min_file = min_file;
                    }
                    else if(strcmp(QUERY_STRINGS[current_query], "C") == 0 || strcmp(QUERY_STRINGS[current_query], "D") == 0){

                        if(max < current->avg_usercount){
                            max = current->avg_usercount;
                            max_file = current->filename_t;
                        }
                        else if(max == current->avg_usercount){
                            if(strcmp(max_file, current->filename_t) > 0)
                                max_file = current->filename_t;
                        }

                        if(min > current->avg_usercount){
                            min = current->avg_usercount;
                            min_file = current->filename_t;
                        }
                        else if(min == current->avg_usercount){
                            if(strcmp(min_file, current->filename_t) > 0)
                                min_file = current->filename_t;
                        }

                        final->max_users = max; //A
                        final->min_users = min; //B
                        final->max_file = max_file;
                        final->min_file = min_file;
                    }
                    else if(strcmp(QUERY_STRINGS[current_query], "E") == 0){
                        int current_population = 0;
                        int index = 0;
                        int flag = 0;

                        current_country = current->country;
                        current_population = current->country_counter;
                        index = 0;
                        flag = 0;

                        while(tcountry_counter[index] != 0){
                            if(strcmp(current_country, tcountry_index[index]) == 0){
                                tcountry_counter[index] = tcountry_counter[index] + current_population;
                                flag = 1;
                                break;
                            }
                            index++;
                        }
                        if(flag == 0){
                            tcountry_index[index] = strdup(current_country);
                            tcountry_counter[index] = current_population;
                        }

                        //We're now going through array to find the max country
                        int max_users = 0;
                        int max_count = 0;
                        int going = 0;
                        while(tcountry_counter[going] != 0){
                            if(max_count < tcountry_counter[going]){
                                max_count = tcountry_counter[going];
                                max_users = going;
                            }
                            going++;
                        }
                        final->country = tcountry_index[max_users];
                        final->country_max = tcountry_counter[max_users];
                    }
                }
            }
        }
        usleep(500);
        if(getRunning == 0){
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
            break;
        }
    }
    return final;
}

static char* name(char* s, int thread_type, int thread_num){
    memset(s, 0, strlen(s));
    if(thread_type == 0){
        strcat(s, "map ");

        char* t = malloc(5);
        sprintf(t, "%d", thread_num);

        strcat(s, t);
    }
    else if(thread_type == 1){
        strcat(s, "reduce");
    }
    return s;
}

static int nfiles(char* dir){
    static int files = 0;
    DIR* ptr = NULL;
    struct dirent *someptr;
    char path[1024];

    if((ptr = opendir(dir)) == NULL)
        return -1;

    while((someptr = readdir(ptr)) != NULL){
        if(strcmp(someptr->d_name, "..") == 0)
            continue;
        else if (strcmp(someptr -> d_name, ".") == 0)
            continue;
        else if(someptr->d_type == DT_DIR){
            path[0] = '\0';
            strncat(path, dir, sizeof(path) - 1);
            strncat(path, "/", sizeof(path) - 1);
            strncat(path, someptr->d_name, sizeof(path) - 1);
            nfiles(path);
        }
        else if(someptr->d_type == DT_REG)
            files++;
    }

    closedir(ptr);

    if(files == 0){
        return 0;
    }
    else
        return files;
}