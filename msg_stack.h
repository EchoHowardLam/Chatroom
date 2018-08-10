#ifndef MSG_STACK_H_INCLUDED
#define MSG_STACK_H_INCLUDED

#include <iostream>
#include <windows.h>
#include <string>
#include <fstream>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

class msg_stack
{
public:
    msg_stack(string stacknam,bool record):cur(0),saveopt(record) //base: The line number where it starts displaying
    {
        boost::unique_lock<boost::mutex> lock(stackusage);
        room_name=stacknam;
        if (saveopt)
        {
            strcpy(save,"save.txt");
            savef.open(save,ios::in);
            savef.seekg(0,ios::end);
            exist=savef&&(savef.tellg()!=0);
            savef.close();
            savef.open(save,ios::out|ios::app);
            if (!savef.good()){error_report("Fail to keep record. Code: 1.");}
            else{
                if (exist){savef<<"\n----------\n";}
                savef<<"Room name: "<<room_name<<" at "<<GetTime(0);
                savef.close();
            }
        }
    }
    void input(string input)
    {
        boost::unique_lock<boost::mutex> lock(stackusage);
        msg[cur++]=input;
        if (cur>=DISPLAY_BOX_HEIGHT){cur=0;}
        if (saveopt)
        {
            savef.open(save,ios::out|ios::app);
            if (!savef.good()){error_report("Fail to keep record. Code: 2.");}
            else{
                savef<<input<<"\n";
                savef.close();
            }
        }
        draw(cur-1);
    }
    void mass_input(string* input,int input_size)
    {
        boost::unique_lock<boost::mutex> lock(stackusage);
        for (int i=0;i<input_size;i++)
        {
            msg[cur++]=input[i];
            if (cur>=DISPLAY_BOX_HEIGHT){cur=0;}
        }
        if (saveopt)
        {
            savef.open(save,ios::out|ios::app);
            if (!savef.good()){error_report("Fail to keep record. Code: 3.");}
            else{
                for (int i=0;i<input_size;i++)
                {
                    savef<<input[i]<<"\n";
                }
                savef.close();
            }
        }
        draw(cur-1);
    }
    void error_report(string input)
    {
        msg[cur++]=input;
        if (cur>=DISPLAY_BOX_HEIGHT){cur=0;}
        draw(cur-1);
    }
    string static concat_msg(string time,string name,string input)
    {
        string tmp=time+name+": "+input;
        return tmp;
    }
    string static concat_sys_msg(string time,string input)
    {
        string tmp=time+input;
        return tmp;
    }
    string static concat_error_msg(string input,int detail)
    {
        stringstream tmp;
        tmp<<input<<detail;
        return tmp.str();
    }
private:
    boost::mutex stackusage;
    string room_name;
    string msg[DISPLAY_BOX_HEIGHT];
    fstream savef;
    short cur; //current position of input stream
    bool exist; //room existence
    char save[10]; //directory of chat content saving
    bool saveopt; //enable or not of the record keeping function
    void draw(int last) //not support multi-line message, last=last displayed message
    {
        int init=last+1;
        if (init>=DISPLAY_BOX_HEIGHT){init=0;}
        for (int i=1;i<=DISPLAY_BOX_HEIGHT;i++) //i indicates of current location of displaying
        {
            gotoxy(1,i+DISPLAY_BOX_BASE);
            cout<<string(80,' ');
            gotoxy(1,i+DISPLAY_BOX_BASE);
            cout<<msg[init++];
            if (init>=DISPLAY_BOX_HEIGHT){init=0;}
        }
    }
};

#endif // MSG_STACK_H_INCLUDED
