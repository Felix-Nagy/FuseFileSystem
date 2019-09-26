//
//  test-myfs.cpp
//  testing
//
//  Created by Oliver Waldhorst on 15.12.17.
//  Copyright © 2017 Oliver Waldhorst. All rights reserved.
//
// since makefile doesn't seem to work, compile with
// g++ -Wall -W -o test.myfs test-myfs.cpp helper.cpp
// then run ./test.myfs


#include "main.cpp"
#include "helper.hpp"

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

using namespace std;

#define MOUNT_SP "../mount/shakespeare.txt"
#define HOME_SP "testfiles/shakespeare.txt"

//before running make sure myfs/mount is mounted and empty
TEST_CASE("testing myfs"){

	int fd1 = open(HOME_SP, O_RDONLY);
    int fd2 = open(MOUNT_SP, O_CREAT | O_APPEND | O_RDWR);
    
    char *buf = new char[256];
    int len = 1;
    while(len > 0){
		len = read(fd1,buf,256);
        write(fd2, buf, len);
    }
    delete[] buf;

    SECTION("testing timestamps"){
        struct stat tsOld;
        stat(HOME_SP, &tsOld);
        struct stat tsNew;
        stat(MOUNT_SP, &tsNew);
        REQUIRE(tsOld.st_ctime < tsNew.st_ctime);
		cout << "old: " << tsOld.st_mtime << "  new: " << tsNew.st_mtime << endl;
        REQUIRE(abs(tsOld.st_mtime - tsNew.st_mtime) < 5);
        REQUIRE(tsOld.st_atime < tsNew.st_atime);
    }
	
	close(fd1);
	close(fd2);

	SECTION("testing file deletion"){
		REQUIRE(remove(MOUNT_SP) == 0);
		
		//should fail, creates new file instead
		REQUIRE(remove(MOUNT_SP) == -1);
	}

	SECTION("testing file appending"){
		int fd = open(MOUNT_SP, O_CREAT | O_APPEND | O_RDWR);
		char* buf = new char[256];
		gen_random(buf,256);
		write(fd, buf, 128);
		char* midway_buf = &buf[128];
		write(fd, midway_buf, 128);
		char* read_buf = new char[256];

		REQUIRE(read(fd,read_buf,256) == 256);
		
		bool appended_correctly = 1;
		while(*buf != '\n'){
			if(*buf != *read_buf){
				appended_correctly = 0;
			}
			buf++;
			read_buf++;
		}

		REQUIRE(appended_correctly == 1);
		delete[] buf;
		delete[] read_buf;
		close(fd);
	}
}
