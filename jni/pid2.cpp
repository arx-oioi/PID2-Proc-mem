#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <vector>
#include <thread>
#include <dirent.h>
#include <cctype>

#include "obfuscate.h"

int byname;

//เก็บข้อมูล lib
std::vector<std::pair<long, long>> get_modules(int pid, const char *module_name) {
    std::vector<std::pair<long, long>> modules;
    FILE *fp;
    char line[1024];
    while (true) {
        char filename[32];
        //เปิด ที่อยู่ ของ address libbase module_name
        snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
        fp = fopen(filename, "r"); 
        // module_name
        if (fp) {
            while (fgets(line, sizeof(line), fp)) {
                if (strstr(line, module_name) && strstr(line, "r-xp"))  //r-xp new line XA
             //   if (strstr(line, module_name)) all line
            {
                    char *pch_start = strtok(line, "-"); //ช่องว่างระหว่าง line 123-129
                    char *pch_end = strtok(NULL, " ");
                    long start = strtoul(pch_start, NULL, 16); // อ่าน Hex Start
                    long end = strtoul(pch_end, NULL, 16);//อ่าน Hex End
                    modules.emplace_back(start, end); //เก็บข้อมูล base address ทั้งหมด 
                }
            }
            fclose(fp);
        }
        if (!modules.empty()) break;
        //usleep(100000); 
    }
    return modules;
}



int getPID(const char* PackageName) {
    while (1) {
        DIR *dir = opendir(OBFUSCATE("/proc"));
        if (!dir) {
            perror(OBFUSCATE("Cannot open /proc"));
            return -1;
        }

        struct dirent *ptr;
        char filepath[256];
        char filetext[128];
        int ipid = 0;

        while ((ptr = readdir(dir)) != NULL) {
            if (ptr->d_type == DT_DIR && isdigit(ptr->d_name[0])) {
                snprintf(filepath, sizeof(filepath), OBFUSCATE ("/proc/%s/cmdline"), ptr->d_name);
                FILE *fp = fopen(filepath, "r");
                if (fp) {
                    fgets(filetext, sizeof(filetext), fp);
                    fclose(fp);

            
                    if (strcmp(filetext, PackageName) == 0) {
                        ipid = atoi(ptr->d_name);
                        break;
                    }
                }
            }
        }

        closedir(dir);

        if (ipid != 0) {
            printf("พบ PID : %d\n", ipid);
            return ipid;
        } else {
           // printf("ไม่พบ PID : %s\n", PackageName);
        }

       sleep(1);
    }
}

//เขียนข้อมูลการเปลี่ยนแปลง
void check_memory_changes(long int startAddress, long int endAddress, const char* module_name) {
    const size_t size = endAddress - startAddress;
    //สร้าง new function
    uint32_t *newhex_values = new uint32_t[size / sizeof(uint32_t)];
    uint32_t *oldhex_values = new uint32_t[size / sizeof(uint32_t)];

    lseek(byname, startAddress, SEEK_SET);//ตรวจสอบ Address
    read(byname, oldhex_values, size); //อ่านHex

    while (true) {
        lseek(byname, startAddress, SEEK_SET);
        read(byname, newhex_values, size);
        // loop Check
        for (size_t i = 0; i < size / sizeof(uint32_t); i++) {
            uint32_t NewValue = newhex_values[i];
            uint32_t OldValue = oldhex_values[i];
            long address = startAddress + i * sizeof(uint32_t);
            //เขียน ระบบส่งออก แบบตึงๆ
            if (NewValue != OldValue) {
                long offset = address - startAddress;
                printf(OBFUSCATE("Lib %s Offset: 0x%LX HexOld: %08X HexChange: %08X\n"),
                module_name, offset, __builtin_bswap32(OldValue), __builtin_bswap32(NewValue));
                oldhex_values[i] = NewValue; 
            }
        }

        //sleep(1);
    }

    delete[] newhex_values;
    delete[] oldhex_values;
}

// ไม่อธิบายแล้ว
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << OBFUSCATE("not support\n");
        return 1;
    }

  
    


    const char* packageName = argv[1];
    std::cout << OBFUSCATE("PID : ") << packageName << std::endl;

    
    if (argc < 2 || strcmp(argv[2], OBFUSCATE("tg:arxmoder")) != 0) {
        printf(OBFUSCATE("Error\n"));
        exit(1);
    }
    
    int pid = getPID(packageName);
    if (pid <= 0) {
        return 1; 
    }

    
    char memPath[64];
    snprintf(memPath, sizeof(memPath), OBFUSCATE("/proc/%d/mem"), pid);
    byname = open(memPath, O_RDWR);
    if (byname == -1) {
        perror(OBFUSCATE("ไม่สามารถเปิด /proc/mem"));
        return 1;
    }

	const char *mname_anogs = OBFUSCATE("libanogs.so");

    auto modules_anogs = get_modules(pid, mname_anogs);
    if (!modules_anogs.empty()) {
        long int startAddress_anogs = modules_anogs.front().first;
        long int endAddress_anogs = modules_anogs.back().second;
        std::thread thread_anogs(check_memory_changes, startAddress_anogs, endAddress_anogs, mname_anogs);
        thread_anogs.detach();
    } else {
        std::cerr << OBFUSCATE("ไม่พบโมดูล ") << mname_anogs << std::endl;
        close(byname);
        return 1;
    }
	
	

    std::cout << OBFUSCATE("TG:ARXMODER") << std::endl;
    std::cin.ignore();
    std::cin.get();

    close(byname);
    return 0;
}
