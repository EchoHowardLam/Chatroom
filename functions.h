#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED

#include <iostream>
#include <sstream>
#include <windows.h>
#include <string>
#include <conio.h>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>

using namespace std;

static COORD point;
static CONSOLE_SCREEN_BUFFER_INFO ConSBuf;
boost::mutex iousage;
bool curmoved=true;
boost::mutex curmovedl;

string static GetTime(int inf) //inf :time format ouput 0 all 1 chattime 2 chattime no bracket
{
    time_t t1=time(NULL);
    struct tm *nPtr=localtime(&t1);
    string now;
    stringstream tmp;
    switch(inf)
    {
    case 0:
        now=asctime(nPtr);break;
    case 1:
        tmp<<"[";
        if (nPtr->tm_hour<10){tmp<<"0";}
        tmp<<nPtr->tm_hour<<":";
        if (nPtr->tm_min<10){tmp<<"0";}
        tmp<<nPtr->tm_min<<"]";
        now=tmp.str();
        break;
    case 2:
        if (nPtr->tm_hour<10){tmp<<"0";}
        tmp<<nPtr->tm_hour<<":";
        if (nPtr->tm_min<10){tmp<<"0";}
        tmp<<nPtr->tm_min;
        now=tmp.str();
        break;
    }
    return now;
}
void static gotoxy(int x,int y)
{
    if ((x>0)|(x<81)|(y>0)|(y<26))
    {
        point.X=x-1,point.Y=y-1;
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE),point);
    }else{exit(3);}
}
void static chmov(int x)
{
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),&ConSBuf);
    point.X=ConSBuf.dwCursorPosition.X;point.Y=ConSBuf.dwCursorPosition.Y;
    if (point.X+x>=0||point.X+x<=79)
    {
        point.X+=x;
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE),point);
    }
}
void static mov_to_index(int index)
{
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),&ConSBuf);
    point.Y=ConSBuf.dwCursorPosition.Y;
    if (index>=0&&index<=79)
    {
        point.X=index;
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE),point);
    }
}

void inline errexit()
{
    system("pause >nul");
    exit(1);
}
string static getip()
{
    char buf[80];
    if (gethostname(buf,sizeof(buf))==SOCKET_ERROR)
    {
        return "Error";
    }
    hostent *data = gethostbyname(buf);
    return inet_ntoa(*((in_addr *)data->h_addr));
}

void draw_room(string roomname,string ip)
{
    system("cls");
    cout<<"Room name: "<<roomname<<"("<<ip<<")";
    gotoxy(50,1);
    cout<<"Time: "<<GetTime(2);
}

void static input(string &target,int y,unsigned int max_input)
{
    int buf=0;
    unsigned int index=0;
    unsigned int total_char=0; //starts from 0
    bool ins_mode=true;
    do
    {
        if (_kbhit())
        {
            boost::unique_lock<boost::mutex> lock(iousage);
            gotoxy(index+1,y);
            if (target.max_size()>=index)
            {
                buf=getch();
                if (buf==224){buf+=getch()*1000;}
                else if (buf==0){buf+=getch()*10;}
                if (buf>=32&&buf<=126)
                {
                    if (index<max_input)
                    {
                        if (ins_mode)
                        {
                            if (total_char<max_input)
                            {
                                target.insert(index,1,buf);
                                cout<<target.substr(index++,string::npos);
                                mov_to_index(index);
                                total_char++;
                            }
                        }else{
                            cout<<(char)buf;
                            target.erase(index,1);
                            target.insert(index++,1,buf);
                        }
                    }
                }else if (buf==8&&index>0){
                    chmov(-1);
                    target.erase(--index,1);
                    cout<<target.substr(index,string::npos)<<" ";
                    total_char--;
                    mov_to_index(index);
                }else if (buf==83224&&index>=0&&index<total_char){
                    target.erase(index,1);
                    cout<<target.substr(index,string::npos)<<" ";
                    total_char--;
                    mov_to_index(index);
                }else if (buf==75224&&index>0){
                    chmov(-1);
                    index--;
                }else if (buf==77224&&index<total_char){
                    chmov(1);
                    index++;
                }else if (buf==82224){
                    ins_mode=(!ins_mode);
                    HANDLE curhandle=GetStdHandle(STD_OUTPUT_HANDLE);
                    CONSOLE_CURSOR_INFO curinfo;
                    curinfo.bVisible=true;
                    if (ins_mode){curinfo.dwSize=25;}
                    else {curinfo.dwSize=50;}
                    SetConsoleCursorInfo(curhandle,&curinfo);
                }
            }else{
                target.resize(target.max_size()+1024,0);
            }
        }
        Sleep(20);
        {
            boost::mutex::scoped_lock lock(curmovedl);
            if (curmoved)
            {
                boost::unique_lock<boost::mutex> lock(iousage);
                curmoved=false;
                gotoxy(index+1,y);
            }
        }
    }while(buf!=13);
}

#endif // FUNCTIONS_H_INCLUDED
