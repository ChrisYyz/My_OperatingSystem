#include <common/types.h>
#include <gdt.h>
#include <memorymanagement.h>
#include <hardwarecomu/interrupts.h>
#include <syscalls.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/driver.h>
#include <hardwarecomu/pci.h>
#include <drivers/vga.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <multitasking.h>
#include <drivers/ata.h>
#include <drivers/amd_am79c973.h>
#include <net/etherframe.h>
#include <net/arp.h>
#include <net/ipv4.h>
#include <net/icmp.h>
#include <net/udp.h>
#include <net/tcp.h>
// #define GRAPHICSMODE

using namespace myos;
using namespace myos::hardwarecomu;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::gui;
using namespace myos::net;

void printf(char* str){
    static uint16_t* VideoMemory = (uint16_t*)0xb8000;

    static uint8_t x=0, y=0;

    for(int i=0 ; str[i] != '\0'; ++i){
        switch(str[i]){
            case '\n':
                y++;
                x = 0;
                break;
            default:
                VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0xFF00) | str[i];
                x++;
        }

        if(x >= 80){
            y++;
            x = 0;
        }

        if(y >= 25){
            for(y = 0 ; y < 25 ; y++){
                for(x = 0 ; x < 80 ; x++){
                    VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | ' ';
                }
            }
            x = 0;
            y = 0;
        }
    }
}

void printfHex(uint8_t key){
    char* foo = "00";
    const char* hex = "0123456789ABCDEF";
    foo[0] = hex[(key >> 4) & 0x0F];
    foo[1] = hex[key & 0x0F];
    printf(foo);
}

class PrintKeyboardEventHandler : public KeyboardEventHandler{
public:
    void OnKeyDown(char c){
        char* foo = " ";
        foo[0] = c;
        printf(foo);
    }     
};

class MouseToConsole : public MouseEventHandler{
    int8_t x, y;
public:
    // initialize mouse
    MouseToConsole(){
    static uint16_t* VideoMemory = (uint16_t*)0xb8000;
    x = 40;
    y = 12;
    VideoMemory[80*y+x] = ((VideoMemory[80*y+x] & 0xF000) >> 4)
                          | ((VideoMemory[80*y+x] & 0x0F00) << 4)
                          | ((VideoMemory[80*y+x] & 0x00FF));
    }
    // mouse move
    void OnMouseMove(int xoffset, int yoffset){
        static uint16_t* VideoMemory = (uint16_t*)0xb8000;
        VideoMemory[80*y+x] = ((VideoMemory[80*y+x] & 0xF000) >> 4)
                            | ((VideoMemory[80*y+x] & 0x0F00) << 4)
                            | ((VideoMemory[80*y+x] & 0x00FF)); 
        x+= xoffset;
        if(x < 0) {x = 0;}
        if(x >= 80) {x = 79;}
        y+= yoffset;
        if( y < 0) {y = 0;}
        if(y >= 25) {y = 24;}

        VideoMemory[80*y+x] = ((VideoMemory[80*y+x] & 0xF000) >> 4)
                            | ((VideoMemory[80*y+x] & 0x0F00) << 4)
                            | ((VideoMemory[80*y+x] & 0x00FF)); 
    }
};

class PrintUDPHandler : public UserDatagramProtocolHandler{
public:
    void HandleUserDatagramProtocolMessage(UserDatagramProtocolSocket* socket, uint8_t* data, uint16_t size)
    {
        char* foo = " ";
        for(int i = 0; i < size; i++)
        {
            foo[0] = data[i];
            printf(foo);
        }
    }     
};

class PrintTCPHandler : public TransmissionControlProtocolHandler{
public:
    bool HandleTransmissionControlProtocolMessage(TransmissionControlProtocolSocket* socket, uint8_t* data, uint16_t size)
    {
        char* foo = " ";
        for(int i = 0; i < size; i++)
        {
            foo[0] = data[i];
            printf(foo);
        }
        return true;
    }     
};


void sysprintf(char* str)
{
    asm("int $0x80" : : "a" (4), "b" (str));
}

void taskA()
{
    while(true)
    {
        sysprintf("A");
    }
}
void taskB()
{
    while(true)
    {
        sysprintf("B");
    }
}





typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;

extern "C" void callConstructors()
{
    for(constructor* i = &start_ctors; i != &end_ctors; i++){
        (*i)();
    }
}

extern "C" void kernelMain(void* multiboot_structure, uint32_t magicnumber){
    
    printf("Hello World!\n");
    
    GlobalDescriptorTable gdt;

    uint32_t* memupper = (uint32_t*)(((size_t)multiboot_structure) + 8);
    size_t heap = 10*1024*1024;
    MemoryManager memoryManager(heap, (*memupper)*1024 - heap - 10*1024);

    printf("\nheap: 0x");
    printfHex((heap >> 24) & 0xFF);
    printfHex((heap >> 16) & 0xFF);
    printfHex((heap >> 8) & 0xFF);
    printfHex((heap) & 0xFF);

    void* allocated = memoryManager.malloc(1024);
    printf("\nheap: 0x");
    printfHex(((size_t)allocated >> 24) & 0xFF);
    printfHex(((size_t)allocated >> 16) & 0xFF);
    printfHex(((size_t)allocated >> 8) & 0xFF);
    printfHex(((size_t)allocated) & 0xFF);
    printf("\n");

    TaskManager taskManager;
    /*
    Task task1(&gdt, taskA);
    Task task2(&gdt, taskB);
    taskManager.AddTask(&task1);
    taskManager.AddTask(&task2);
    */
    InterruptManager interrupts(0x20, &gdt, &taskManager);
    SyscallHandler syscalls(&interrupts, 0x80);

    printf("Initializing Hardware, I\n");   

    #ifdef GRAPHICSMODE
        Desktop desktop(320, 200, 0x00, 0xA8, 0x00);
    #endif

        DriverManager drvManager;

    #ifdef GRAPHICSMODE
        KeyboardDriver keyboard(&interrupts, &desktop);
    #else
        PrintKeyboardEventHandler pkbhandler;
        KeyboardDriver keyboard(&interrupts, &pkbhandler);
    #endif
        drvManager.AddDriver(&keyboard);

    #ifdef GRAPHICSMODE
        MouseDriver mouse(&interrupts, &desktop);
    #else
        MouseToConsole mhandler;
        MouseDriver mouse(&interrupts, &mhandler);
    #endif
        drvManager.AddDriver(&mouse);

        PeripheralComponentInterconnectController PCIcontroller;
        PCIcontroller.SelectDrivers(&drvManager, &interrupts);
    #ifdef GRAPHICSMODE
        VideoGraphicsArray vga;
    #endif
        printf("Initializing Hardware, II\n");
        drvManager.ActivateAll();

        printf("Initializing Hardware, III\n");
    #ifdef GRAPHICSMODE
        vga.SetMode(320, 200, 8);

        Window win1(&desktop, 10, 10, 20, 20, 0xA8,0x00,0x00);
        desktop.AddChild(&win1);
        Window win2(&desktop, 40, 15, 30, 30, 0x00,0x00,0xA8);
        desktop.AddChild(&win2);
    #endif

        // interrupt 14
        AdvancedTechnologyAttachment ata0m(0x1F0, true);
        printf("ATA Primary Master: ");
        ata0m.Identify();
        AdvancedTechnologyAttachment ata0s(0x1F0, false);
        printf("ATA Primary Slave: ");
        ata0s.Identify();

        /* ata test
        char* atabuffer = "hello hard drive";
        ata0s.Write28(0,(uint8_t*)atabuffer,16);
        ata0s.Flush();
        ata0s.Read28(0,(uint8_t*)atabuffer,16);
        */

        // interrupt 15
        AdvancedTechnologyAttachment ata1m(0x170, true);
        AdvancedTechnologyAttachment ata1s(0x170, false);

        //thrid: 0x1E8
        //fourth: 0x168

        uint8_t ip1 = 10, ip2 = 0, ip3 = 2, ip4 = 15;
        uint32_t ip_be = ((uint32_t)ip4 << 24) 
                       | ((uint32_t)ip3 << 16)
                       | ((uint32_t)ip2 << 8)
                       | ((uint32_t)ip1);

        uint8_t gip1 = 10, gip2 = 0, gip3 = 2, gip4 = 2;
        uint32_t gip_be = ((uint32_t)gip4 << 24) 
                       | ((uint32_t)gip3 << 16)
                       | ((uint32_t)gip2 << 8)
                       | ((uint32_t)gip1);
        uint8_t subnetMask1 = 255, subnetMask2 = 255, subnetMask3 = 255, subnetMask4 = 0;
        uint32_t subnetMask_be = ((uint32_t)subnetMask4 << 24) 
                       | ((uint32_t)subnetMask3 << 16)
                       | ((uint32_t)subnetMask2 << 8)
                       | ((uint32_t)subnetMask1);
        
        amd_am79c973* eth0 = (amd_am79c973*)(drvManager.drivers[2]);


        eth0->SetIPAddress(ip_be);

        EtherFrameProvider etherframe(eth0);

        AddressResolutionProtocol arp(&etherframe);

        InternetProtocolProvider ipv4(&etherframe, &arp, gip_be, subnetMask_be);
        InternetControlMessageProtocol icmp(&ipv4);
        UserDatagramProtocolProvider udp(&ipv4);
        TransmissionControlProtocolProvider tcp(&ipv4);


        interrupts.Activate();

        printf("\n\n\n\n");

        arp.BroadcastMACAddress(gip_be);

        PrintTCPHandler tcphandler;
        TransmissionControlProtocolSocket* tcpsocket = tcp.Connect(gip_be, 1234);
        tcp.Bind(tcpsocket, &tcphandler);
        tcpsocket->Send((uint8_t*)"Hello TCP!", 10);

        //icmp.RequestEchoReply(gip_be);


        //PrintUDPHandler udphandler;
        //UserDatagramProtocolSocket* udpsocket = udp.Connect(gip_be, 1234);
        //udp.Bind(udpsocket, &udphandler);
        //udpsocket->Send((uint8_t*)"Hello UDP!", 10);

        //UserDatagramProtocolSocket* udpsocket = udp.Listen(1234);
        //udp.Bind(udpsocket, &udphandler);


        while(1)
        {
        #ifdef GRAPHICSMODE
            desktop.Draw(&vga);
        #endif
        }
}
