#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>
#include "potato.h"
#include "socket.h"
using namespace std;

class Ringmaster{
public:
    vector<int> player_fds;
    vector<int> player_ports;
    vector<string> player_addrs;
    int num_players;
    int num_hops;
    Ringmaster(int num_players, int num_hops): num_players(num_players), num_hops(num_hops){}
    void connectPlayers(){
        //connect each player and its neighbors
        //ringmater just send each player its neighbor's ip & port.
        for(int id = 0; id < num_players; ++id) {
          //int n_id = (id + 1) % num_players;
            int n_id = (id + 1) >= num_players ? (id + 1 - num_players): id+1;
            int n_port = player_ports[n_id];
          char n_ip[80];
          memset(n_ip, 0, sizeof(n_ip));
          strcpy(n_ip, player_addrs[n_id].c_str());
          //send current player its neighbor's ip& port
          send(player_fds[id], &n_port, sizeof(n_port), 0);
          send(player_fds[id], &n_ip, sizeof(n_ip), 0);
        }
    }
    void connectRings(int ring_fd) {
        for(int player_id = 0; player_id < num_players; ++player_id) {
          string ip_addr;
          int port;
            int player_fd = connection(ring_fd, &ip_addr);
          send(player_fd, &player_id, sizeof(player_id), 0);
          send(player_fd, &num_players, sizeof(num_players), 0);
          //ringmaster recv player's port
          recv(player_fd, &port, sizeof(port), 0);
          player_fds.push_back(player_fd);
          player_ports.push_back(port);
          player_addrs.push_back(ip_addr);
          cout << "Player " << player_id << " is ready to play" << endl;
        }
    }
    void printStart(int num_players, int num_hops) {
        cout << "Potato Ringmaster" << endl;
        cout << "Players = " << num_players << endl;
        cout << "Hops = " << num_hops << endl;
    }
    void play() {
        Potato p;
        p.num_hops = num_hops;
        //send the potato and waiter for the potato to come back
        if(num_hops != 0) {//if hops=0, don't send the potato
          //create random number
          //srand((unsigned int)time(NULL) + num_players);
          int random  = rand() % num_players;
          //send potato to the first player
          send(player_fds[random], &p, sizeof(p), 0);
          cout << "Ready to start the game, sending potato to player " << random << endl;

          //receive last potato
          //using select() to see which of your file descriptors is ready to send you content
          fd_set readfds;
            int max_ele = player_fds[0];
            for(int player_fd: player_fds) {
                max_ele = max(max_ele, player_fd);
            }
         //int nfds = 1 + *max_element(player_fds.begin(), player_fds.end());
          FD_ZERO(&readfds);
          for(int player_fd: player_fds) {
              FD_SET(player_fd, &readfds);
          }
          select(1+ max_ele, &readfds, NULL, NULL, NULL);
          for(int player_fd: player_fds) {
            if(FD_ISSET(player_fd, &readfds)) {
              recv(player_fd, &p, sizeof(p), MSG_WAITALL);
              break;
            }
          }
         }
        for(int player_fd: player_fds) {
          send(player_fd, &p.num_hops, sizeof(p.num_hops), 0);
        }
       //print trace massage
       p.print_Trace();
       //close the process
       for(int player_fd: player_fds) {
         close(player_fd);
        }
    }
};

int main(int argc, char *argv[]) {
  if(argc != 4) {
  cerr << "Invalid Format!" << endl;
  return EXIT_FAILURE;
  }
  const char *ring_port = argv[1];
  int num_players = atoi(argv[2]);
  int num_hops = atoi(argv[3]);
  
  //restrict: num_player >1; 0 <= num_hops <= 512
  if(num_players <= 1) {
    cerr << "Invalid num_player" << endl;
    return EXIT_FAILURE;
  }
  if(num_hops < 0 || num_hops > 512) {
    cerr << "Invalid num_hops" << endl;
    return EXIT_FAILURE;
  }
    
  //initialize server
    int ring_fd = invoke_server(ring_port);
    Ringmaster ring(num_players, num_hops);
    ring.printStart(num_players, num_hops);
    //accept connection from players
    ring.connectRings(ring_fd);
    ring.connectPlayers();
    ring.play();

close(ring_fd);
return 0;
}
