#include <pcap.h>
#include <vector>
#include <ctime>
#include <ncurses.h>
#include "CRadioHeader.h"
#include "CBeaconFrame.h"
#include "CWirelessManagement.h"
#include "CAirodump.h"


void usage()
{
    std::cerr << "syntax : airodump <interface>" << std::endl;
    std::cerr << "sample : airodump mon0" << std::endl;
}

void printErrorInterface(char* interface)
{
    std::cerr << interface << " can't access" << std::endl;
    std::cerr << "Check input InterFace" << std::endl;
}

void printAirodump_ng(std::vector<CAirodump> airodumplist)
{
    move(0,0);
    std::time_t t =std::time(nullptr);
    std::tm* now = std::localtime(&t);

    printw("CH %d\t][ Elapsed: %d s ][ %d-%02d-%02d %d:%d\n", 8, 0, now->tm_year + 1900, now->tm_mon+1, now->tm_mday, now->tm_hour, now->tm_min);
    printw("\n");
    printw("BSSID\t\tPWR\tBecons\t#Data,\t#/s\tCH\tMB\tENC\tCIPHER\tAUTH\tESSID\n");

    for(int i = 0; i < airodumplist.size(); i++)
    {
        printw("%s\t%d\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%s\t%s\n", airodumplist[i].BSSID.c_str(), airodumplist[i].PWR, airodumplist[i].Beacons, 
            airodumplist[i].Data, airodumplist[i].SlashSec, airodumplist[i].CH, airodumplist[i].MB, airodumplist[i].ENC.c_str(), 
            airodumplist[i].CIPHER.c_str(), airodumplist[i].AUTH.c_str(), airodumplist[i].ESSID.c_str());
    }
    refresh();
    getch();

}

int main(int argc, char* argv[])
{
    // 만약 인자로 인터페이스를 제공하지 않았을 경우
    if(argc < 2)
    {
        usage();
        return -1;
    }

    char* interface = argv[1];
    char strErr[PCAP_ERRBUF_SIZE];
    pcap_t* handle = pcap_open_live(interface, BUFSIZ, 1, 1, strErr);
    
    // 해당 인터페이스가 없는 경우
    if(handle == nullptr)
    {
        printErrorInterface(interface);
        return -1;
    }
    
    // Ncurses 객체 초기화
    initscr();

    std::vector<CAirodump> wirelessList;

    // 무한반복을 하며 진행
    while(true)
    {
        struct pcap_pkthdr* header;
        const u_char* packet;

        int res = pcap_next_ex(handle, &header, &packet);
        if(res == 0) continue;
        if (res == PCAP_ERROR || res == PCAP_ERROR_BREAK) {
			printf("pcap_next_ex return %d(%s)\n", res, pcap_geterr(handle));
			break;
		}
        C80211RadioHeader radioHeader = C80211RadioHeader(packet);

        int radioHeaderBackLocation = radioHeader.get80211Length();
        
        u_char beaconSign = { 0x80 };
        char no[5] = "NOPE";
        
        // BeaconFrame인지 확인
        if(memcmp(&packet[radioHeaderBackLocation], &beaconSign, 1) == 0)
        {
            // BeaconFrame이 맞다면 Beacon정보를 다룰 객체 생성
            C80211BeaconFrame wirelessbeacon = C80211BeaconFrame(&packet[radioHeaderBackLocation]);
            CWirelessManagement wirelessManagement = CWirelessManagement(&packet[radioHeaderBackLocation + 24]);
            CAirodump airoElement = CAirodump(wirelessbeacon.getBSSID(), radioHeader.getsignalPower(), 0, wirelessManagement.getChannel(), 
            0, no, no, no, wirelessManagement.getSSID());

            bool checker = false;
            for(int i= 0; i < wirelessList.size(); i++)
            {
                if(wirelessList[i].BSSID == airoElement.BSSID)
                {   
                    checker = true;
                    wirelessList[i].updateAiroDetail(airoElement.PWR, airoElement.SlashSec, airoElement.MB);
                    wirelessList[i].updateBeacons();
                }
            }
            if(checker == false)
                wirelessList.push_back(airoElement);
            printAirodump_ng(wirelessList);
        }
        else
            continue;
    }
    getch();
    endwin();
    return 0;
}