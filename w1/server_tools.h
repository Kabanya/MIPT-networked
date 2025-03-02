#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <string>
#include <queue>


struct Client
{
  sockaddr_in addr;
  std::string identifier;
};

struct MathProblem
{
  std::string problem;
  int answer;
};

struct MathDuel
{
  Client challenger;
  Client opponent;
  int answer;
  std::string problem;
  bool active;
  bool solved;
};

extern std::queue<Client> duelQueue;
extern std::vector<MathDuel> activeDuels;

std::string client_to_string(const Client& client);

void msg_to_all_clients(int sfd, const std::vector<Client>& clients, const std::string& message);

void msg_to_client(int sfd, const Client& client, const std::string& message);

MathProblem generate_math_problem();

void start_math_duel(int sfd, const Client& challenger, const Client& opponent, std::vector<Client>& all_clients);

bool is_in_duel(const Client& client, MathDuel** current_duel = nullptr);

void server_input_processing(int sfd, std::vector<Client>& clients);

void mathduel(std::string message, Client currentClient, int sfd, std::vector<Client> &clients);
