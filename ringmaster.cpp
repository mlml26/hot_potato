#include <iostream>
#include <cstring>
#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>
#include "potato.h"
#include "socket.h"
using namespace std;
int main(int argc, char *argv[]) {
  // ringmaster <port_num> <num_player><>
  if(argc != 4) {
  cerr << "Invalid Format!" << endl;
  return 1;
  }
  const char *ring_port = argv[1];
  int num_players = atoi(argv[2]);
  int num_hops = atoi(argv[3]);
  //restrict: num_player >1; 0 <= num_hops <= 512
  if(num_players <= 1) {
    cerr << "Invalid num_player" << endl;
    return 1;
  }
  if(num_hops < 0 || num_hops > 512) {
    cerr << "Invalid num_hops" << endl;
    return 1;
  }
  cout << "Potato Ringmaster" << endl;
  cout << "Players = " << num_players << endl;
  cout << "Hops = " << num_hops << endl;

  //initialize server
  //  const char *ring_port = argv[1];
  int ring_fd = invoke_server(ring_port);

  //accept connection from players
  vector<int> player_fds;
  vector<int> player_ports;
  vector<string> player_addrs;
  
  for(int player_id = 0; player_id < num_players; ++player_id) {
    string ip_addr;
    int port;
    int player_fd = connection(ring_fd, &ip_addr);
    //cout << player_fd <<" is connected" << endl;
    //ip:成功与server connect的client地址结构
      //return:能与server进行数据通信的socket对应的fd
      //int connection(int client_socket_fd, string * ip)
    //ringmaster send player_id and num_players
    send(player_fd, &player_id, sizeof(player_id), 0);
    send(player_fd, &num_players, sizeof(num_players), 0);
    //ringmaster recv player's port
    recv(player_fd, &port, sizeof(port), 0);
    player_fds.push_back(player_fd);
    player_ports.push_back(port);
    player_addrs.push_back(ip_addr);
    //    cout << "Port" << port <<endl;
    //cout << player_fd << ", " << port << ", "<< ip_addr  << endl;
    cout << "Player " << player_id << " is ready to play" << endl;
  }
  //connect each player and its neighbors
  //ringmater just send each player its neighbor's ip & port.
  for(int id = 0; id < num_players; ++id) {
    int neighbor_id = (id + 1) % num_players;
    int neighbor_port = player_ports[neighbor_id];
    char neighbor_ip[100];
    memset(neighbor_ip, 0, sizeof(neighbor_ip));
    strcpy(neighbor_ip, player_addrs[neighbor_id].c_str());
    //send current player its neighbor's ip& port
    send(player_fds[id], &neighbor_port, sizeof(neighbor_port), 0);
    send(player_fds[id], &neighbor_ip, sizeof(neighbor_ip), 0);
  }
  

//start potato game
Potato p;

p.num_hops = num_hops; 
//send the potato and waiter for the potato to come back
if(num_hops != 0) {//if hops=0, don't send the potato
  //create random number
  srand((unsigned int)time(NULL) + num_players);
  int random  = rand() % num_players;
  //send potato to the first player
  send(player_fds[random], &p, sizeof(p), 0);
  cout << "Ready to start the game, sending potato to player " << random << endl;

  //receive last potato
  //using select() to see which of your file descriptors is ready to send you content
  fd_set readfds;
  int nfds = 1 + *max_element(player_fds.begin(), player_fds.end());
  FD_ZERO(&readfds);
  for(int player_fd: player_fds) {
    FD_SET(player_fd, &readfds);
  }
  select(nfds, &readfds, NULL, NULL, NULL);
  for(int i = 0; i < num_players; ++i){//player_fd: player_fds) {
    if(FD_ISSET(player_fds[i], &readfds)) {
     // cout << "Recieve back player: " << i << endl;
      recv(player_fds[i], &p, sizeof(p), MSG_WAITALL);
      break;
    }
  }
 }

//send potato with num_hops 0 to all players to shut down

 for(int player_fd: player_fds) {
   send(player_fd, &p.num_hops, sizeof(p.num_hops), 0);
 }
//print trace massage
p.print_Trace();
//close the process
for(int player_fd: player_fds) {
  close(player_fd);
 }
close(ring_fd);
return 0;
}
