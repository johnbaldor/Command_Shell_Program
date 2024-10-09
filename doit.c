
/* The mini-shell has basic functionality like a regular Linux shell, which can execute commands like cd or ls,
and can also set a custom prompt, run commands in the background, and display background jobs. It doesn't 
have more advanced features like Bash or command history. It also has limits on input size and the number of arguments. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

void execute_command(int argc, char *argv[]);
void check_background_jobs();

struct BackgroundJob {
    pid_t pid;
    char command[128];
};
struct BackgroundJob background_jobs[32];
int num_background_jobs = 0;



int main(int argc, char *argv[]) {
    if (argc > 1) {
        execute_command(argc - 1, &argv[1]);
        return 0;
    }

    char input[128];
    char *args[32];
    char prompt[32] = "==>";
    char *token;
    int arg_count;

    while (1) {
        printf("%s ", prompt);
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        arg_count = 0;
        token = strtok(input, " ");
        while (token != NULL && arg_count < 32 - 1) {
            args[arg_count] = token;
            arg_count++;
            token = strtok(NULL, " ");
        }
        args[arg_count] = NULL;

        if (arg_count == 0) {
            continue;
        } else if (strcmp(args[0], "exit") == 0) {
            if (num_background_jobs > 0) {
                printf("There are still background jobs running.\n");
                continue;
            }
            break;
        } else if (strcmp(args[0], "cd") == 0) {
            if (arg_count < 2) {
                fprintf(stderr, "cd: missing argument\n");
            } else {
                if (chdir(args[1]) != 0) {
                    perror("cd");
                }
            }
        } else if (strcmp(args[0], "set") == 0 && arg_count == 4 && strcmp(args[1], "prompt") == 0 && strcmp(args[2], "=") == 0) {
            strncpy(prompt, args[3], sizeof(prompt) - 1);
            prompt[sizeof(prompt) - 1] = '\0';
        } else if (strcmp(args[0], "jobs") == 0) {
            printf("Background Jobs:\n");
            if (num_background_jobs == 0) {
                puts("none");
            } else {
                for (int i = 0; i < num_background_jobs; i++) {
                    printf("[%d] %d %s\n", i + 1, background_jobs[i].pid, background_jobs[i].command);
                }
            }
        } else {
            check_background_jobs();
            execute_command(arg_count, args);
        }
    }
    return 0;
}



void check_background_jobs() {
    int status;
    for (int i = 0; i < num_background_jobs; i++) {
        pid_t result = waitpid(background_jobs[i].pid, &status, WNOHANG);
        if (result == 0) {
            continue;
        } else if (result == -1) {
            perror("waitpid");
        } else {
            printf("[%d] %d %s Completed\n", i + 1, background_jobs[i].pid, background_jobs[i].command);
            for (int j = i; j < num_background_jobs - 1; j++) {
                background_jobs[j] = background_jobs[j + 1];
            }
            num_background_jobs--;
            i--;
        }
    }
}



void execute_command(int argc, char *argv[]) {
    int background = 0;

    if (argc > 0 && strcmp(argv[argc - 1], "&") == 0) {
        argv[argc - 1] = NULL;
        background = 1;
        argc--;
    }

    struct timeval start_time, end_time;
    struct rusage usage;
    int status;

    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork error");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        execvp(argv[0], argv);
        perror("Execvp error");
        exit(EXIT_FAILURE);
    } else {
        if (background) {
            background_jobs[num_background_jobs].pid = pid;
            strncpy(background_jobs[num_background_jobs].command, argv[0], sizeof(background_jobs[num_background_jobs].command) - 1);
            background_jobs[num_background_jobs].command[sizeof(background_jobs[num_background_jobs].command) - 1] = '\0';
            num_background_jobs++;
            printf("[%d] %d\n", num_background_jobs, pid);
        } else {
            gettimeofday(&start_time, NULL);

            if (waitpid(pid, &status, 0) < 0) {
                perror("Wait error");
                exit(EXIT_FAILURE);
            }

            gettimeofday(&end_time, NULL);
            getrusage(RUSAGE_CHILDREN, &usage);

            long wct = ((end_time.tv_sec - start_time.tv_sec) * 1000) + ((end_time.tv_usec - start_time.tv_usec) / 1000);

            printf("Elapsed wall-clock time: %ld ms\n", wct);
            printf("CPU time used (user): %ld ms\n", (usage.ru_utime.tv_sec * 1000) + (usage.ru_utime.tv_usec / 1000));
            printf("CPU time used (system): %ld ms\n", (usage.ru_stime.tv_sec * 1000) + (usage.ru_stime.tv_usec / 1000));
            printf("Involuntary context switches: %ld\n", usage.ru_nivcsw);
            printf("Voluntary context switches: %ld\n", usage.ru_nvcsw);
            printf("Major page faults: %ld\n", usage.ru_majflt);
            printf("Minor page faults: %ld\n", usage.ru_minflt);
            printf("Maximum resident set size: %ld KB\n", usage.ru_maxrss);

            if (WIFEXITED(status)) {
                printf("Child process exited with status %d\n", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("Child process terminated by signal %d\n", WTERMSIG(status));
            } else {
                printf("Child process terminated abnormally\n");
            }
        }
    }
}
