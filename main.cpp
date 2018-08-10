#define LINEMAX 60 //Max char in one line
#define DISPLAY_BOX_BASE 1 //The line number where the display zone starts
#define DISPLAY_BOX_HEIGHT 20 //Max height of output area
char COM_VERSION[]="6";
char VERSION[]="6";

#include <iostream>
#include <windows.h>
#include <string>
#include <time.h>
#include <fstream>
#include <forward_list>
#include "functions.h"
#include "msg_stack.h"

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>

#include <Winsock2.h>

/*
PUBLISH-CHECK
version-number 3 places
# marks

TO-do
copy & paste

cli: version
svr: 01/01 roomname
cli: pass / username
svr: 01
*/
using namespace std;

struct user_setting;
void server_proc(user_setting& usetting);
void client_init(user_setting& usetting);
void time_update();
void user_input(msg_stack& data_stack,SOCKET& server_sock);
void client_recv(msg_stack& data_stack,SOCKET& server_sock,bool alert);

struct user_setting
{
    bool alert=true;
    bool keep_record=false;
};
class user_data
{
public:
    user_data(SOCKET isock,string name):verifylv(3),username(name),ip(getip()),verified(true),prilv(0){sock=isock;}
    user_data(SOCKET isock):verifylv(0),username(""),verified(false),prilv(2){sock=isock;}
    bool verify(string name){if (name.length()<=10){verified=true;verifylv=3;username=name;return true;}return false;}
    bool is_verified(){return verified;}
    bool admin(){if (prilv!=0){prilv=1;return true;}return false;}
    bool user(){if (prilv!=0){prilv=2;return true;}return false;}
    bool mute(){if (prilv!=0){muted=true;}return muted;}
    bool unmute(){if (prilv!=0){muted=false;}return muted;}
    bool is_muted(){return muted;}
    bool is_op(){return (prilv==0);}
    bool is_admin(){return (prilv==1);}
    bool is_pri(){return (prilv<2);}
    int privilege(){return prilv;}
    void removethis(){removing=true;verified=false;}
    friend bool needremove(user_data& checked);
    SOCKET sock;
    int verifylv; //0 none 1 version 2 password 3 username
    string username;
    string ip;
private:
    bool muted=false;
    bool removing=false;
    bool verified;
    short prilv; //0 op 1 admin 2 user
};
bool needremove(user_data& checked){return checked.removing;}

int main()
{
    system("title Chatroom v.6");
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(1,1),&wsadata)!=0)
    {
        return 1;
    }
     if (LOBYTE(wsadata.wVersion)!= 1||HIBYTE(wsadata.wVersion )!= 1)
    {
        WSACleanup();
        return 1;
    }
    cout<<"Loading settings...\n";
    user_setting usetting;

    fstream settingfp;
    char settingfn[]="settings.dat";
    settingfp.open(settingfn,ios::in|ios::binary);
    settingfp.seekg(0,ios::end);
    bool settingexist=settingfp&&(settingfp.tellg()!=0);
    if (settingexist)
    {
        settingfp.seekg(0,ios::beg);
        settingfp.read((char *)&usetting,sizeof(user_setting));
    }
    settingfp.close();

    char option;
    do
    {
        system("cls");
        cout<<"Chatroom v.6\n";
        cout<<"         (c) Echo \n";
        cout<<"Options\n";
        cout<<"(1) Join a room\n";
        cout<<"(2) Start a room\n";
        cout<<"(3) Settings\n";
        cout<<"(4) Information\n";
        cout<<"(5) Exit\n";
        cout<<"Action: ";
        cin>>option;
        cin.ignore(INT_MAX,'\n');
        cin.clear();
        switch (option)
        {
        case '1':
            client_init(usetting);
            break;
        case '2':
            server_proc(usetting);
            break;
        case '3':
            {
                int key_press;
                bool changed=false;
                system("cls");
                cout<<"Setting::..\n";
                cout<<"(1)Alert: ";
                if (usetting.alert){cout<<"on";}else {cout<<"off";}
                cout<<"\n(2)Keep record: ";
                if (usetting.keep_record){cout<<"on";}else {cout<<"off";}
                cout<<"\n(3)Back\n";
                do
                {
                    key_press=getch();
                    if (key_press==49)
                    {
                        changed=true;
                        gotoxy(11,2);
                        usetting.alert=(!usetting.alert);
                        if (usetting.alert){cout<<"on ";}else {cout<<"off";}
                    }else if (key_press==50)
                    {
                        changed=true;
                        gotoxy(17,3);
                        usetting.keep_record=(!usetting.keep_record);
                        if (usetting.keep_record){cout<<"on ";}else {cout<<"off";}
                    }
                    gotoxy(1,5);
                }while (key_press!=51);
                if (changed)
                {
                    cout<<"Saving the settings...";
                    settingfp.open(settingfn,ios::out);
                    if (!settingfp.good()){cout<<"Fail to save new settings.";system("pause >nul");}
                    settingfp.write((char*)&usetting,sizeof(user_setting));
                    settingfp.close();
                }
            }
            break;
        case '4':
            system("cls");
            cout<<"Chatroom v6\n";
            cout<<"   Not compatible with any previous versions\n";
            cout<<"Built on 2/7/2014\n\n";
            cout<<"Programmer:\n";
            cout<<"   Echo\n";
            cout<<"\nTesters:\n";
            cout<<"   Carson\n";
            cout<<"   Allen\n";
            cout<<"   Samuel Lo\n";
            cout<<"   Ivan\n";
            cout<<"   Leo Lee\n";
            cout<<"   Kenny Kong\n";
            cout<<"\nUsed port\n";
            cout<<"   14999 Fixed port for Server\n";
            cout<<"Press any key to return...\n";
            system("pause >nul");
            break;
        case '5':
            cout<<"Exiting...";
            break;
        default:
            cout<<"Incorrect input.\n";
            cout<<"Press any key to return...\n";
            system("pause >nul");
            break;
        }
    }while (option!='5');
    WSACleanup();
    return 0;
}

void server_proc(user_setting& usetting)
{
    //Room name
    string roomname;
    system("cls");
    cout<<"Create a room::..\n";
    cout<<"Room IP: "<<getip()<<endl;
    cout<<endl;
    do
    {
        cout<<"Room name: ";
        getline(cin,roomname);
        if (roomname.length()>20)
        {
            cout<<"The length of room name cannot excess 20.\n";
        }
    }while (roomname.length()>20);
    //Maximum connections
    unsigned int max_connection;
    while (1)
    {
        cout<<"Maximum connections allowed(1-20): ";
        string tmp;
        getline(cin,tmp);
        stringstream tmp2(tmp);
        if (!(tmp2>>max_connection))
        {
            cout<<"Please enter a number.\n";
        }else if((max_connection<1)||(max_connection>20)){
            cout<<"Please enter a value between 1 and 20.\n";
        }else{break;}
    }
    //Password
    string password;
    do
    {
        cout<<"Password(or leave this blank): ";
        getline(cin,password);
        if (password.length()>10)
        {
            cout<<"The length of the password cannot excess 10.\n";
        }
    }while (password.length()>10);
    //OP user name
    string username;
    do
    {
        cout<<"User name: ";
        getline(cin,username);
        if (username.length()>10)
        {
            cout<<"The length of the user name cannot excess 10.\n";
        }
    }while (username.length()>10);
    //Initialisation of sockets
    int addr_len=sizeof(SOCKADDR);
    bool reuseaddr=true;

    SOCKET listen_sock=socket(AF_INET,SOCK_STREAM,0);
    if (listen_sock==INVALID_SOCKET){cout<<"Starting listener socket error: "<<WSAGetLastError();errexit();}
    SOCKADDR_IN svr_addr;
    svr_addr.sin_family=AF_INET;
    svr_addr.sin_port=htons(14999);
    svr_addr.sin_addr.S_un.S_addr=htonl(INADDR_ANY);
    if (setsockopt(listen_sock,SOL_SOCKET,SO_REUSEADDR,(const char*)&reuseaddr,sizeof(bool))==SOCKET_ERROR)
    {cout<<"Setting multiple address error: "<<WSAGetLastError();errexit();}

    if (bind(listen_sock,(SOCKADDR*)&svr_addr,addr_len)==SOCKET_ERROR)
    {
        if (WSAGetLastError()==10013){cout<<"Binding port error: Permission not acquired.";}
        else {cout<<"Binding port error.";}
        errexit();
    }
    if (listen(listen_sock,5)==SOCKET_ERROR){cout<<"Listening port error "<<WSAGetLastError();errexit();}

    SOCKET self_sock=socket(AF_INET,SOCK_STREAM,0);
    if (self_sock==INVALID_SOCKET){cout<<"Starting server side socket error: "<<WSAGetLastError();errexit();}
    SOCKADDR_IN sc_addr;
    sc_addr.sin_family=AF_INET;
    sc_addr.sin_port=htons(14999);
    sc_addr.sin_addr.S_un.S_addr=inet_addr("127.0.0.1");
    if (connect(self_sock,(SOCKADDR*)&sc_addr,addr_len)==SOCKET_ERROR){cout<<"Self-connecting error "<<WSAGetLastError();errexit();}

    SOCKADDR_IN op_addr;
    SOCKET op_sock=accept(listen_sock,(SOCKADDR*)&op_addr,&addr_len);
    //Drawing room & special msg
    draw_room(roomname,getip());
    msg_stack display(roomname,usetting.keep_record);
    display.input(msg_stack::concat_sys_msg(GetTime(1),"You have started a room."));
    display.input(msg_stack::concat_sys_msg(GetTime(1),"You are now the operator. Type -help for help"));
    boost::thread Tclient_recv(boost::bind(&client_recv,boost::ref(display),boost::ref(self_sock),usetting.alert));
    boost::thread Tuser_input(boost::bind(&user_input,boost::ref(display),boost::ref(self_sock)));
    boost::thread Ttime_update(&time_update);
    //Loop
    user_data op_data(op_sock,username);
    forward_list<user_data> userlist;
    userlist.push_front(op_data);
    fd_set socklist;
    SOCKET newsock;
    SOCKADDR_IN newaddr;
    unsigned int maxsock; //greatest sock number
    unsigned int nowsock=1;unsigned int onholdsock=0;
    if (listen_sock>op_sock){maxsock=listen_sock;}
    else {maxsock=op_sock;}
    int command_list_size=7;
    string command_list[]={"who", //remember to update the int above
                           "mute ",
                           "unmute ",
                           "kick ",
                           "inf ",
                           "admin ",
                           "user "};
    while (1)
    {
        //Setting fd_set
        FD_ZERO(&socklist);
        FD_SET(listen_sock,&socklist);
        for (auto cur_user=userlist.begin();cur_user!=userlist.end();cur_user++)
        {
            FD_SET(cur_user->sock,&socklist);
        }
        //Select
        if ((select(maxsock+1,&socklist,NULL,NULL,NULL))==-1)
        {
            boost::unique_lock<boost::mutex> lock(iousage);
            display.input(msg_stack::concat_error_msg("Select error ",WSAGetLastError()));
            errexit();
        }
        //Check new connection
        if (FD_ISSET(listen_sock,&socklist))
        {
            newsock=accept(listen_sock,(SOCKADDR*)&newaddr,&addr_len);
            //# error handling
            if ((newsock!=INVALID_SOCKET)&&(onholdsock<5))
            {
                if (newsock>maxsock){maxsock=newsock;}
                user_data newdata(newsock);
                newdata.ip=inet_ntoa(newaddr.sin_addr);
                userlist.push_front(newdata);
                onholdsock++;
            }else{closesocket(newsock);}
        }
        //Registered msg
        for (auto cur_user=userlist.begin();cur_user!=userlist.end();cur_user++)
        {
            if (FD_ISSET(cur_user->sock,&socklist))
            {
                if (cur_user->is_verified())
                {
                    char recvmsg[LINEMAX];
                    string sysmsg;
                    int sendtarget=-1; //0 all 1 cur_user 2 access denied ~none
                    if (recv(cur_user->sock,recvmsg,sizeof(recvmsg)+1,0)<=0)
                    {
                        closesocket(cur_user->sock);
                        cur_user->removethis();
                        nowsock--;
                        sysmsg=GetTime(1)+"User "+cur_user->username+" has disconnected.";
                        sendtarget=0;
                    }else{
                        string tmpmsg=recvmsg;
                        if (tmpmsg.length()>0)
                        {
                            if (tmpmsg[0]!='-')
                            {
                                if (!cur_user->is_muted())
                                {
                                    sysmsg=msg_stack::concat_msg(GetTime(1),cur_user->username,recvmsg);
                                    sendtarget=0;
                                }else{
                                    sysmsg="You are muted.";
                                    sendtarget=1;
                                }
                            }else{
                                size_t commandpos;
                                for (int i=0;i<command_list_size;i++)
                                {
                                    commandpos=tmpmsg.find(command_list[i]);
                                    if (commandpos==1)
                                    {
                                        switch (i)
                                        {
                                        case 0:
                                            {
                                                stringstream tmp_nowsock;
                                                tmp_nowsock<<nowsock;
                                                sysmsg="There ";
                                                if (nowsock!=1){sysmsg+="are ";}
                                                else {sysmsg+="is ";}
                                                sysmsg+=tmp_nowsock.str();
                                                if (nowsock!=1){sysmsg+=" users in the room. Who are: ";}
                                                else {sysmsg+=" user in the room. Who is: ";}
                                                for (auto test_user=userlist.begin();test_user!=userlist.end();test_user++)
                                                {
                                                    if (test_user->is_verified())
                                                    {
                                                        sysmsg+=test_user->username;
                                                        sysmsg+=".";
                                                    }
                                                }
                                                sendtarget=1;
                                            }
                                            break;
                                        case 1:
                                            if (tmpmsg.length()>command_list[i].length()+1)
                                            {
                                                string search_user=tmpmsg.substr(command_list[i].length()+1);
                                                for (auto test_user=userlist.begin();test_user!=userlist.end();test_user++)
                                                {
                                                    if ((test_user->is_verified())&&(test_user->username==search_user))
                                                    {
                                                        if (cur_user->privilege()<test_user->privilege())
                                                        {
                                                            test_user->mute();
                                                            sysmsg=GetTime(1)+"User "+test_user->username+" has been muted by "+cur_user->username+".";
                                                            sendtarget=0;
                                                        }else{
                                                            sendtarget=2;
                                                        }
                                                        break;
                                                    }
                                                }
                                            }
                                            break;
                                        case 2:
                                            if (tmpmsg.length()>command_list[i].length()+1)
                                            {
                                                string search_user=tmpmsg.substr(command_list[i].length()+1);
                                                for (auto test_user=userlist.begin();test_user!=userlist.end();test_user++)
                                                {
                                                    if ((test_user->is_verified())&&(test_user->username==search_user))
                                                    {
                                                        if (cur_user->privilege()<test_user->privilege())
                                                        {
                                                            test_user->unmute();
                                                            sysmsg=GetTime(1)+"User "+test_user->username+" has been unmuted by "+cur_user->username+".";
                                                            sendtarget=0;
                                                        }else{
                                                            sendtarget=2;
                                                        }
                                                        break;
                                                    }
                                                }
                                            }
                                            break;
                                        case 3:
                                            if (tmpmsg.length()>command_list[i].length()+1)
                                            {
                                                string search_user=tmpmsg.substr(command_list[i].length()+1);
                                                for (auto test_user=userlist.begin();test_user!=userlist.end();test_user++)
                                                {
                                                    if ((test_user->is_verified())&&(test_user->username==search_user))
                                                    {
                                                        if (cur_user->privilege()<test_user->privilege())
                                                        {
                                                            closesocket(test_user->sock);
                                                            test_user->removethis();
                                                            nowsock--;
                                                            sysmsg=GetTime(1)+"User "+test_user->username+" has been kicked by "+cur_user->username+".";
                                                            sendtarget=0;
                                                        }else{
                                                            sendtarget=2;
                                                        }
                                                        break;
                                                    }
                                                }
                                            }
                                            break;
                                        case 4:
                                            if (cur_user->is_op())
                                            {
                                                if (tmpmsg.length()>command_list[i].length()+1)
                                                {
                                                    string search_user=tmpmsg.substr(command_list[i].length()+1);
                                                    for (auto test_user=userlist.begin();test_user!=userlist.end();test_user++)
                                                    {
                                                        if ((test_user->is_verified())&&(test_user->username==search_user))
                                                        {
                                                            sysmsg="User "+test_user->username;
                                                            if (test_user->is_muted()){sysmsg+="(muted)";}
                                                            sysmsg+=" is ";
                                                            if (test_user->is_op()){sysmsg+="an operator";}
                                                            else if (test_user->is_admin()){sysmsg+="an admin";}
                                                            else {sysmsg+="a user";}
                                                            sysmsg+=" from "+test_user->ip;
                                                            sendtarget=1;
                                                            break;
                                                        }
                                                    }
                                                }
                                            }else{
                                                sendtarget=2;
                                            }
                                            break;
                                        case 5:
                                            if (cur_user->is_op())
                                            {
                                                if (tmpmsg.length()>command_list[i].length()+1)
                                                {
                                                    string search_user=tmpmsg.substr(command_list[i].length()+1);
                                                    if (cur_user->username!=search_user)
                                                    {
                                                        for (auto test_user=userlist.begin();test_user!=userlist.end();test_user++)
                                                        {
                                                            if ((test_user->is_verified())&&(test_user->username==search_user))
                                                            {
                                                                if (test_user->admin())
                                                                {
                                                                    sysmsg=GetTime(1)+"User "+test_user->username+" is now an admin.";
                                                                    sendtarget=0;
                                                                }
                                                                break;
                                                            }
                                                        }
                                                    }else{
                                                        sendtarget=2;
                                                    }
                                                }
                                            }else{
                                                sendtarget=2;
                                            }
                                            break;
                                        case 6:
                                            if (cur_user->is_op())
                                            {
                                                if (tmpmsg.length()>command_list[i].length()+1)
                                                {
                                                    string search_user=tmpmsg.substr(command_list[i].length()+1);
                                                    if (cur_user->username!=search_user)
                                                    {
                                                        for (auto test_user=userlist.begin();test_user!=userlist.end();test_user++)
                                                        {
                                                            if ((test_user->is_verified())&&(test_user->username==search_user))
                                                            {
                                                                if (test_user->user())
                                                                {
                                                                    sysmsg=GetTime(1)+"User "+test_user->username+" is now a user.";
                                                                    sendtarget=0;
                                                                }
                                                                break;
                                                            }
                                                        }
                                                    }else{
                                                        sendtarget=2;
                                                    }
                                                }
                                            }else{
                                                sendtarget=2;
                                            }
                                            break;
                                        }
                                        break;
                                    }
                                }
                            }
                        }else{
                            if (!cur_user->is_muted())
                            {
                                sysmsg=msg_stack::concat_msg(GetTime(1),cur_user->username,recvmsg);
                                sendtarget=0;
                            }else{
                                sysmsg="You are muted.";
                                sendtarget=1;
                            }
                        }
                    }
                    switch (sendtarget)
                    {
                    case 0:
                        for (auto call_user=userlist.begin();call_user!=userlist.end();call_user++)
                        {
                            if (call_user->is_verified())
                            {
                                send(call_user->sock,sysmsg.c_str(),sysmsg.length()+1,0);
                            }
                        }
                        break;
                    case 1:
                        send(cur_user->sock,sysmsg.c_str(),sysmsg.length()+1,0);
                        break;
                    case 2:
                        sysmsg="Access denied.";
                        send(cur_user->sock,sysmsg.c_str(),sysmsg.length()+1,0);
                        break;
                    }
                }else{
                    switch (cur_user->verifylv)
                    {
                    case 0:
                        char recvver[10];
                        if (recv(cur_user->sock,recvver,11,0)<=0)
                        {
                            closesocket(cur_user->sock);
                            cur_user->removethis();
                            onholdsock--;
                        }else if(((*recvver)!=(*VERSION))&&((*recvver)!=(*COM_VERSION))){
                            closesocket(cur_user->sock);
                            cur_user->removethis();
                            onholdsock--;
                        }else{
                            string sendres=roomname;
                            if (password.length()<=0)
                            {
                                cur_user->verifylv=2;
                                sendres.insert(0,"0");
                            }else{
                                cur_user->verifylv=1;
                                sendres.insert(0,"1");
                            }
                            send(cur_user->sock,sendres.c_str(),sendres.length()+1,0);
                        }
                        break;
                    case 1:
                        char recvpwd[10];
                        if (recv(cur_user->sock,recvpwd,11,0)<=0)
                        {
                            closesocket(cur_user->sock);
                            cur_user->removethis();
                            onholdsock--;
                        }else if(recvpwd!=password){
                            closesocket(cur_user->sock);
                            cur_user->removethis();
                            onholdsock--;
                        }else{
                            string indicator="1";
                            cur_user->verifylv=2;
                            send(cur_user->sock,indicator.c_str(),10,0);
                        }
                        break;
                    case 2:
                        char recvusn[10];
                        onholdsock--; //successful or not will cause 1 less verifying user
                        if (recv(cur_user->sock,recvusn,11,0)<=0)
                        {
                            closesocket(cur_user->sock);
                            cur_user->removethis();
                        }else{
                            string new_username=recvusn;
                            if (!cur_user->verify(new_username))
                            {
                                closesocket(cur_user->sock);
                                cur_user->removethis();
                            }else if(nowsock>=max_connection){
                                string errmsg="The room is full currently.";
                                send(cur_user->sock,errmsg.c_str(),errmsg.length()+1,0);
                                closesocket(cur_user->sock);
                                cur_user->removethis();
                            }else{
                                bool name_valid=true;
                                if (cur_user->username.length()<=0)
                                {
                                    string errmsg="Your username is too short.";
                                    send(cur_user->sock,errmsg.c_str(),errmsg.length()+1,0);
                                    closesocket(cur_user->sock);
                                    cur_user->removethis();
                                    name_valid=false;
                                }else{
                                    for (auto test_user=userlist.begin();test_user!=userlist.end();test_user++)
                                    {
                                        if ((test_user->is_verified())&&(cur_user->username==test_user->username)&&(test_user!=cur_user))
                                        {
                                            string errmsg="Your username has been used.";
                                            send(cur_user->sock,errmsg.c_str(),errmsg.length()+1,0);
                                            closesocket(cur_user->sock);
                                            cur_user->removethis();
                                            name_valid=false;
                                            break;
                                        }
                                    }
                                }
                                if (name_valid)
                                {
                                    nowsock++;
                                    string sysmsg=GetTime(1)+"User "+cur_user->username+" has connected.";
                                    for (auto call_user=userlist.begin();call_user!=userlist.end();call_user++)
                                    {
                                        if (call_user->is_verified())
                                        {
                                            send(call_user->sock,sysmsg.c_str(),sysmsg.length()+1,0);
                                        }
                                    }
                                }
                            }
                        }
                        break;
                    }
                }
            }
        }
        userlist.remove_if(needremove);
    }
    Tclient_recv.join();
    Tuser_input.join();
    Ttime_update.join();
    return;
}

void client_init(user_setting& usetting)
{
    string targetIP;
    system("cls");
    cout<<"Join a room::..\n";
    cout<<"Local IP: "<<getip()<<endl;
    cout<<endl;
    cout<<"Server IP: ";
    getline(cin,targetIP);

    int addr_len=sizeof(SOCKADDR);

    SOCKET sockconnect=socket(AF_INET,SOCK_STREAM,0);
    if (sockconnect==INVALID_SOCKET){cout<<"Starting target socket error: "<<WSAGetLastError();errexit();}
    SOCKADDR_IN target_addr;
    target_addr.sin_family=AF_INET;
    target_addr.sin_port=htons(14999);
    target_addr.sin_addr.S_un.S_addr=inet_addr(targetIP.c_str());
    if (connect(sockconnect,(SOCKADDR*)&target_addr,addr_len)==SOCKET_ERROR)
    {
        cout<<"Fail to connect. Code: "<<WSAGetLastError()<<endl;
        cout<<"Press any key to return...\n";
        system("pause >nul");
        return;
    }
    if (send(sockconnect,VERSION,11,0)<=0)
    {
        closesocket(sockconnect);
        cout<<"Fail to send version number.\n";
        cout<<"Server may be busy at the moment\n";
        cout<<"Press any key to return...\n";
        system("pause >nul");
        return;
    }
    char roomname_buf[22];
    if (recv(sockconnect,roomname_buf,22,0)<=0)
    {
        closesocket(sockconnect);
        cout<<"Version number not matched.\n";
        cout<<"Press any key to return...\n";
        system("pause >nul");
        return;
    }
    string roomname=roomname_buf;
    bool have_password=(roomname[0]=='1');
    roomname=roomname.substr(1);
    cout<<"Version verified.\n";
    cout<<"Room name: "<<roomname<<endl;
    if (have_password)
    {
        string password;
        do
        {
            cout<<"Password: ";
            getline(cin,password);
            if (password.length()>10)
            {
                cout<<"The length of the password cannot excess 10.\n";
            }
        }while (password.length()>10);
        if (send(sockconnect,password.c_str(),11,0)<=0)
        {
            closesocket(sockconnect);
            cout<<"Fail to send password.\n";
            cout<<"Press any key to return...\n";
            system("pause >nul");
            return;
        }
        char checkpwd[10];
        if (recv(sockconnect,checkpwd,10,0)<=0)
        {
            closesocket(sockconnect);
            cout<<"Disconnected.\n";
            cout<<"Press any key to return...\n";
            system("pause >nul");
            return;
        }
        string indicator="1";
        if (checkpwd!=indicator)
        {
            closesocket(sockconnect);
            cout<<"Undesired message received from server.\n";
            cout<<"Possibility of invalid connection\n";
            cout<<"Press any key to return...\n";
            system("pause >nul");
            return;
        }
        cout<<"Password verified.\n";
    }
    string username;
    do
    {
        cout<<"User name: ";
        getline(cin,username);
        if (username.length()>10)
        {
            cout<<"The length of the user name cannot excess 10.\n";
        }
    }while (username.length()>10);
    if (send(sockconnect,username.c_str(),11,0)<=0)
    {
        closesocket(sockconnect);
        cout<<"Fail to send username.\n";
        cout<<"The room may be full at the moment.\n";
        cout<<"Press any key to return...\n";
        system("pause >nul");
        return;
    }

    draw_room(roomname,targetIP);
    msg_stack display(roomname,usetting.keep_record);
    display.input(msg_stack::concat_sys_msg(GetTime(1),"You are now a user. Type -help for help."));
    boost::thread Tclient_recv(boost::bind(&client_recv,boost::ref(display),boost::ref(sockconnect),usetting.alert));
    boost::thread Tuser_input(boost::bind(&user_input,boost::ref(display),boost::ref(sockconnect)));
    boost::thread Ttime_update(&time_update);
    Tclient_recv.join();
    Tuser_input.join();
    Ttime_update.join();
}

void time_update()
{
    string now;
    time_t curtime;
    tm *local_time;
    while (1)
    {
        now=GetTime(2);
        {
            boost::unique_lock<boost::mutex> lock(iousage);
            boost::unique_lock<boost::mutex> lock2(curmovedl);
            curmoved=true;
            gotoxy(56,1);
            cout<<now;
        }
        Sleep(1000);
        if (GetTime(2)==now)
        {
            curtime=time(NULL);
            local_time=localtime(&curtime);
            Sleep((60-local_time->tm_sec)*1000);
        }
    }
}

void user_input(msg_stack& data_stack,SOCKET& server_sock)
{
    string in;
    int help_size=14;
    string help[]={"Available commands:",
                   "Operator:",
                   "   -user <username>",
                   "   -admin <username>",
                   "   -inf <username>",
                   "",
                   "Admin:",
                   "   -kick <username>",
                   "   -mute <username>",
                   "   -unmute <username>",
                   "",
                   "User:",
                   "   -help",
                   "   -who"};
    while (1)
    {
        {
            boost::unique_lock<boost::mutex> lock(iousage);
            gotoxy(1,DISPLAY_BOX_BASE+DISPLAY_BOX_HEIGHT+1);
            cout<<string(60,' ');
            gotoxy(1,DISPLAY_BOX_BASE+DISPLAY_BOX_HEIGHT+1);
        }
        input(in,DISPLAY_BOX_BASE+DISPLAY_BOX_HEIGHT+1,LINEMAX);
        if (in!="-help")
        {
            if (send(server_sock,in.c_str(),in.length()+1,0)<=0)
            {
                break;
            }
        }else{
            boost::unique_lock<boost::mutex> lock(iousage);
            boost::unique_lock<boost::mutex> lock2(curmovedl);
            data_stack.mass_input(help,help_size);
        }
        in.clear();
    }
}

void client_recv(msg_stack& data_stack,SOCKET& server_sock,bool alert)
{
    while (1)
    {
        char msg[80];
        if (recv(server_sock,msg,sizeof(msg)+1,0)<=0)
        {
            closesocket(server_sock);
            boost::unique_lock<boost::mutex> lock(iousage);
            boost::unique_lock<boost::mutex> lock2(curmovedl);
            if (alert){cout<<"\a";}
            data_stack.input(data_stack.concat_sys_msg(GetTime(1),"You have disconnected."));
            curmoved=true;
            break;
        }
        {
            boost::unique_lock<boost::mutex> lock(iousage);
            boost::unique_lock<boost::mutex> lock2(curmovedl);
            if (alert){cout<<"\a";}
            data_stack.input(msg);
            curmoved=true;
        }
    }
}
