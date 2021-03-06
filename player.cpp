#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <algorithm>
#include <arpa/inet.h>
#include <vector>
#include "potato.h"
#include "socket.h"
using namespace std;
 vector<int> ringConnect(int ring_fd){
     vector<int> playerInfo;
     // Receive player_id, num_players
    int num_players, player_id;
    recv(ring_fd, &player_id, sizeof(player_id),0);
    recv(ring_fd, &num_players, sizeof(num_players), 0);
     playerInfo.push_back(player_id);//[0]
     playerInfo.push_back(num_players);//[1]
    cout << "Connected as player " << player_id << " out of " << num_players << "total players" << endl;
    int player_server_fd = invoke_server("");
    int player_port = getPort(player_server_fd);
     playerInfo.push_back(player_server_fd);//[2]
    //cout << "player_port" << player_port << endl;
    send(ring_fd, &player_port, sizeof(player_port), 0);
     return playerInfo;
}
vector<int> neighborConnect(int ring_fd, int player_server_fd) {
    vector<int> n_fds;
    int n_port;
    char n_ip[100];
    recv(ring_fd, &n_port, sizeof(n_port), 0);
    recv(ring_fd, &n_ip, sizeof(n_ip), 0);
    //player work as client, connect to right neighbor
    char n_port_char[9];
    sprintf(n_port_char, "%d", n_port);
    n_fds.push_back(invoke_client(n_ip, n_port_char));//[0]:right
    //player works as server, accept the left neighbor's connection
    string ip_addr;
    n_fds.push_back(connection(player_server_fd, &ip_addr));//[1]:left
    return n_fds;
}
void process_potato(vector<int> three_fds, int p_id, int num_players, Potato & p, int recieve) {
    
    //if hops = 1, send it to ringmaster
    if (p.num_hops == 1) {
  //cout << "num_hops= " << p.num_hops << endl;
  //cout << "I'm the last one" <<endl;
        p.num_hops--;
        p.num_path++;
        p.trace[p.num_path-1] = p_id;
    cout << "I'm it" << endl;
    send(three_fds[2], &p, sizeof(p), 0);//ring_fd
        //cout << "I'm it" << endl;
    }
    //if hops >1, send it to neigbors randomly, send potato to neighbor
    else{
        p.num_hops--;
        p.num_path++;
        p.trace[p.num_path-1] = p_id;
        int next = rand() % 2;
        if(next) {
            send(three_fds[1], &p, sizeof(p), 0);//right_fd
            int next_id = (p_id + 1) % num_players;
            cout << "Sending potato to " << next_id << endl;
        }
        else {
            
            send(three_fds[0], &p, sizeof(p), 0);//left_fd
            int next_id = (p_id - 1 + num_players) % num_players;
            cout << "Sending potato to " << next_id << endl;
        }
    }
}
void run(vector<int> three_fds, int p_id, int num_players) {
    fd_set readfds;
    int max_ele = three_fds[0];
    for(int three_fd: three_fds) {
        max_ele = max(max_ele, three_fd);
    }
    //int nfds = 1 + *max_element(connected_fds.begin(), connected_fds.end());
    
    //receive the potato
    while(1){
        Potato p;
        FD_ZERO(&readfds);
        for(int three_fd: three_fds) {
            FD_SET(three_fd, &readfds);
        }
        select(1+ max_ele, &readfds, NULL, NULL, NULL);
    
        int recieve;
        //receive the potato
        for(int three_fd: three_fds) {
            if (FD_ISSET(three_fd, &readfds)) {
                recieve=  recv(three_fd, &p, sizeof(p), MSG_WAITALL);
                break;
            }
        }
        //if hops = 0, shut down the game
        if(p.num_hops == 0 || recieve == 0) break;
        process_potato(three_fds, p_id, num_players, p, recieve);
    }
}
int main(int argc, char *argv[]) {
  if(argc != 3) {
    cerr << "Invalid Format!" << endl;
    return 1;
  }
  //player(server) establish connection with ringmaster(client)
  const char *hostname = argv[1];
  const char *port = argv[2];
    int ring_fd = invoke_client(hostname, port);
    vector<int> playerInfo = ringConnect(ring_fd);
    //int player_id = playerInfo[0];
    //int num_players = playerInfo[1];
    //int player_server_fd = playerInfo[2];
  //recv neighbor's ip&port from ringmaster
    vector<int> nInfo = neighborConnect(ring_fd, playerInfo[2]);
    //int right_fd =nInfo[0];
    //int left_fd = nInfo[1];
  //start the game
    vector<int> three_fds;//[0]
    three_fds.push_back(nInfo[1]);
    three_fds.push_back(nInfo[0]);
    three_fds.push_back(ring_fd);
    run(three_fds, playerInfo[0], playerInfo[1]);
    //shut down the process left_neighbor_fd, right_neighbor_fd, ring_socket_fd
        close(nInfo[1]);
        close(nInfo[0]);
        close(ring_fd);
        return 0;
}
