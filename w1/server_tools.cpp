#include <iostream>

#include "server_tools.h"

std::vector<MathDuel> activeDuels;
std::queue<Client> duelQueue;

std::string client_to_string(const Client& client)
{
  return std::string(inet_ntoa(client.addr.sin_addr)) + ":" + 
         std::to_string(ntohs(client.addr.sin_port));
}

void msg_to_all_clients(int sfd, const std::vector<Client>& clients, const std::string& message)
{
  for (const Client& client : clients)
  {
    socklen_t slen = sizeof(sockaddr_in);
    sendto(sfd, message.c_str(), message.size(), 0, (struct sockaddr*)&client.addr, slen);
  }
  printf("msg to all clients: %s\n", message.c_str());
}

void msg_to_client(int sfd, const Client& client, const std::string& message)
{
  socklen_t slen = sizeof(sockaddr_in);
  sendto(sfd, message.c_str(), message.size(), 0, (struct sockaddr*)&client.addr, slen);
  printf("msg to client (%s): %s\n", client_to_string(client).c_str(), message.c_str());
}

MathProblem generate_math_problem()
{
  static bool initialized = false;
  if (!initialized) {
    srand(time(NULL));
    initialized = true;
  }
  
  int num1 = rand() % 30 + 1;
  int num2 = rand() % 30 + 1;
  int num3 = rand() % 30 + 1;
  
  int op1 = rand() % 3;
  int op2 = rand() % 3;
  
  int result = 0;
  std::string problem;
  
  if (op1 == 0) { // Addition
    problem = std::to_string(num1) + " + " + std::to_string(num2);
    result = num1 + num2;
  } else if (op1 == 1) { // Subtraction
    problem = std::to_string(num1) + " - " + std::to_string(num2);
    result = num1 - num2;
  } else { // Multiplication
    problem = std::to_string(num1) + " * " + std::to_string(num2);
    result = num1 * num2;
  }
  
  if (op2 == 0) { // Addition
    problem += " + " + std::to_string(num3);
    result += num3;
  } else if (op2 == 1) { // Subtraction
    problem += " - " + std::to_string(num3);
    result -= num3;
  } else { // Multiplication
    problem = "(" + problem + ") * " + std::to_string(num3);
    result *= num3;
  }
  
  problem += " = ?";
  
  MathProblem mathProblem;
  mathProblem.problem = problem;
  mathProblem.answer = result;
  
  return mathProblem;
}

void start_math_duel(int sfd, const Client& challenger, const Client& opponent, std::vector<Client>& all_clients)
{
  
  MathProblem mathProblem = generate_math_problem();
  
  MathDuel duel;
  duel.challenger = challenger;
  duel.opponent = opponent;
  duel.answer = mathProblem.answer;
  duel.problem = mathProblem.problem;
  duel.active = true;
  duel.solved = false;
  
  activeDuels.push_back(duel);
  
  std::string announcement = "MATH DUEL STARTING: " + client_to_string(challenger) + 
                             " vs " + client_to_string(opponent);
  msg_to_all_clients(sfd, all_clients, announcement);
  
  std::string challenge = "MATH DUEL PROBLEM: " + mathProblem.problem + "\nAnswer with /ans <your answer>";
  msg_to_client(sfd, challenger, challenge);
  msg_to_client(sfd, opponent, challenge);
  
  std::cout << "Math duel started between " << client_to_string(challenger) 
            << " and " << client_to_string(opponent) 
            << ", answer: " << mathProblem.answer << std::endl;
}

bool is_in_duel(const Client& client, MathDuel** current_duel)
{
  for (size_t i = 0; i < activeDuels.size(); i++)
  {
    MathDuel& duel = activeDuels[i];
    if (!duel.active || duel.solved) continue;
    
    if ((client.addr.sin_addr.s_addr == duel.challenger.addr.sin_addr.s_addr &&
         client.addr.sin_port == duel.challenger.addr.sin_port) ||
        (client.addr.sin_addr.s_addr == duel.opponent.addr.sin_addr.s_addr &&
         client.addr.sin_port == duel.opponent.addr.sin_port))
    {
      if (current_duel) *current_duel = &duel;
      return true;
    }
  }
  return false;
}

void server_input_processing(int sfd, std::vector<Client>& clients)
{
  std::string input;
  while (true)
  {
    printf(">");
    std::getline(std::cin, input);
    if (!input.empty())
    {
      std::string broadcastMsg = "SERVER: " + input;
      msg_to_all_clients(sfd, clients, broadcastMsg);
    }
  }
}

void mathduel(std::string message, Client currentClient, int sfd, std::vector<Client> &clients)
{
  if (message == "/mathduel")
  {
    if (is_in_duel(currentClient))
    {
      msg_to_client(sfd, currentClient, "You are already in a math duel!");
      return;
    }

    if (duelQueue.empty())
    {
      duelQueue.push(currentClient);
      msg_to_client(sfd, currentClient, "Waiting for an opponent...");
      msg_to_all_clients(sfd, clients, "CHAT (Server): " + currentClient.identifier + " wants math duel! Type /mathduel to join.");
    }
    else
    {
      Client opponent = duelQueue.front();
      duelQueue.pop();

      bool opponentExists = false;
      for (const Client& client : clients)
      {
        if (client.addr.sin_addr.s_addr == opponent.addr.sin_addr.s_addr && 
          client.addr.sin_port == opponent.addr.sin_port &&
          !(client.addr.sin_addr.s_addr == currentClient.addr.sin_addr.s_addr && 
            client.addr.sin_port == currentClient.addr.sin_port))
        {
          opponentExists = true;
          break;
        }
      }

      if (opponentExists)
      {
        start_math_duel(sfd, currentClient, opponent, clients);
      }
      else
      {
        duelQueue.push(currentClient);
        msg_to_client(sfd, currentClient, "Waiting for an opponent...");
        msg_to_all_clients(sfd, clients, "CHAT (Server): " + currentClient.identifier + " wants math duel! Type /mathduel to join.");
      }
    }
  }

  if (message.length() > 5 && message.substr(0, 5) == "/ans ")
  {
    std::string ans_str = message.substr(5);
    int user_answer;
    bool valid_input = true;

    try 
    {
      user_answer = std::stoi(ans_str);
    } 
    catch (const std::exception& e) 
    {
      msg_to_client(sfd, currentClient, "Invalid answer format. Use /ans <number>");
      valid_input = false;
    }

    if (valid_input) 
    {
      MathDuel* currentDuel = nullptr;
      if (!is_in_duel(currentClient, &currentDuel) || !currentDuel)
      {
          msg_to_client(sfd, currentClient, "You are not in an active math duel!");
      }
      else
      {
        if (user_answer == currentDuel->answer)
        {
          std::string winner_id = client_to_string(currentClient);
          std::string announcement = "MATH DUEL RESULT: " + winner_id + " won duel with true answer: " + 
          std::to_string(currentDuel->answer) + "!";
          msg_to_all_clients(sfd, clients, announcement);
          
          currentDuel->solved = true;
          currentDuel->active = false;
        }
        else
        {
          msg_to_client(sfd, currentClient, "Wrong answer! Try again.");
        }
      }
    }
  }
}

void cleanup_inactive_duels(std::vector<MathDuel>& activeDuels)
{
  for (size_t i = 0; i < activeDuels.size();)
  {
    if (!activeDuels[i].active || activeDuels[i].solved)
    {
      if (i < activeDuels.size() - 1)
        activeDuels[i] = activeDuels.back();
      activeDuels.pop_back();
    }
    else
    {
      i++;
    }
  }
}
