/*
    LinzerSchnitte Midi 2 RDS - Midi Interface for generating RDS for Linzer Schnitter
    Copyright (C) 2014  Josh Gardiner

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>

    Complie with
	$ gcc -lasound -o midi2rds midi2rds.c
*/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <mysql.h>
#include <signal.h>
#include "i2c_bitbang.h"

#define FIELDIDX_RDSPATTERNS_ADDRESS 1
#define FIELDIDX_RDSPATTERNS_COMMAND 2
#define FIELDIDX_RDSPATTERNS_DATA 3
#define FIELDIDX_RDSPATTERNS_PROCESSED 4
#define FIELDIDX_RDSPATTERNS_ID 5

int issue_command(int address, int value, int data) 
{
    printf("\nRDSCMD: %2.2x %4.4x %4.4x\n", value, address, data);
    LS_CMD(value, address, data);

    return 0;
}


int running;

void quit_handler(int s){
    printf("Caught signal %d\n",s);

    exit(1);

    running = 0;
}

int main () {

    struct sigaction sigIntHandler;
    int headerIdx, fieldIdx;

    setbuf(stdout, NULL);

    if (gpioSetup() != OK)
    {
        dbgPrint(DBG_INFO, "gpioSetup failed. Exiting\n");
        return 1;
    }

    running = 1;

    sigIntHandler.sa_handler = quit_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);

    MYSQL *mysqlConRet;
    MYSQL *mysqlConnection = NULL;

    char* hostname = "schnittsrv";
    char* userid = "root";
    char* password = "pentium4711";
    char* db = "sparks-web";

    mysqlConnection = mysql_init(NULL);

    mysqlConRet = mysql_real_connect( mysqlConnection,
                                      hostname,
                                      userid,
                                      password,
                                      db,
                                      0,
                                      NULL,
                                      0);

    if (mysqlConRet == NULL)
    {
        exit(1);
    }

    printf("connection info: %s \n", mysql_get_host_info(mysqlConnection));
    printf("client info: %s \n", mysql_get_client_info());
    printf("server info: %s \n", mysql_get_server_info(mysqlConnection));

    int mysqlStatus = 0;
    MYSQL_RES *mysqlResult = NULL;

    MYSQL_ROW mysqlRow;
    MYSQL_FIELD *mysqlFields;
    my_ulonglong numRows;
    unsigned int numFields;

    while (running)
    {

        if(mysqlResult)
        {
            mysql_free_result(mysqlResult);
            mysqlResult = NULL;
        }

        char* query = "SELECT * FROM commands_rdspatterns WHERE processed=0 ORDER BY timestamp";
        mysqlStatus = mysql_query(mysqlConnection, query);

        if (mysqlStatus)
        {
            printf("query error!");
        }
        else
        {
            mysqlResult = mysql_store_result(mysqlConnection);
        }

        if (mysqlResult)
        {
            numRows = mysql_num_rows(mysqlResult);
            numFields = mysql_num_fields(mysqlResult);
            printf("Number of rows=%u  Number of fields=%u \n",numRows,numFields);
        }
        else
        {
            printf("Result set is empty");
        }

        mysqlFields = mysql_fetch_fields(mysqlResult);
        for (headerIdx = 0; headerIdx < numFields; headerIdx++)
        {
            printf("%s\t", mysqlFields[headerIdx].name);
        }
        printf("\n");

        while (mysqlRow = mysql_fetch_row(mysqlResult))
        {
            for (fieldIdx = 0; fieldIdx < numFields; fieldIdx++)
            {
                printf("%s\t", mysqlRow[fieldIdx] ? mysqlRow[fieldIdx] : "NULL");
            }

            issue_command(    atoi(mysqlRow[FIELDIDX_RDSPATTERNS_ADDRESS]),
                              atoi(mysqlRow[FIELDIDX_RDSPATTERNS_COMMAND]),
                              atoi(mysqlRow[FIELDIDX_RDSPATTERNS_DATA]));
            sleep(1);
            char *update;
            sprintf(update, "UPDATE commands_rdspatterns SET processed=1 WHERE rdspatterns_id=%d", 
                    atoi(mysqlRow[FIELDIDX_RDSPATTERNS_ID]));
            mysqlStatus = mysql_query(mysqlConnection, update);
            sleep(1);
            printf("\n");

            if (mysqlStatus)
            {
                printf("update error!");
            }
        }

        if (mysqlResult)
        {
            mysql_free_result(mysqlResult);
            mysqlResult = NULL;
        }

        sleep(3);

    }

    printf("CLOSING\n");
    mysql_close(mysqlConnection);

    gpioCleanup();
    return (0);
}
