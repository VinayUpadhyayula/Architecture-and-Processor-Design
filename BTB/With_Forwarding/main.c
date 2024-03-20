/*
 * main.c
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apex_cpu.h"

int
main(int argc, char const *argv[])
{
    APEX_CPU *cpu;
    int command =  0;
    fprintf(stderr, "APEX CPU Pipeline Simulator v%0.1lf\n", VERSION);

    if (argc != 2)
    {
        fprintf(stderr, "APEX_Help: Usage %s <input_file>\n", argv[0]);
        exit(1);
    }
    else if(argc == 2){
    while(1){
    printf("Enter a command(1-6):\n1.Initialize\n2.Simulate <no of cycles>\n3.Single_step\n4.Display\n5.ShowMem <address>\n6.quit\n");
    scanf("%d", &command);
    
    if(command == 1 ){
        cpu = APEX_cpu_init(argv[1]);
    }
    else if(command >=2 && command <=5){
        if(cpu ==NULL || cpu->pc < 4000){
            printf("Error!!CPU has not been initialized yet, please perform Intialize step first!!");
        }
       else
       {
        APEX_cpu_run(cpu, command);
        }
    }
    else if(command == 6)
    {
        APEX_cpu_stop(cpu);
            exit(1);
    }
    else
    {
        printf("Invalid Command!! Please enter any command as per the menu..");
    }
    }
    }
    return 0;
}