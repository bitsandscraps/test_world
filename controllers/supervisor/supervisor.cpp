// Author(s):         Inbae Jeong
// Maintainer:        Chansol Hong (cshong@rit.kaist.ac.kr)

#include "constants.hpp"
#include "supervisor.hpp"
#include "game.hpp"
#include "wamp_router.hpp"

#include <chrono>
#include <thread>
#include <cstdlib>

constexpr double MAX_POSTURE[] = { constants::FIELD_LENGTH / 2.2,
                                   constants::FIELD_WIDTH / 2.2,
                                   2 * constants::PI };
constexpr double MIN_POSTURE[] = { -1 * constants::FIELD_LENGTH / 2.2,
                                   -1 * constants::FIELD_WIDTH / 2.2,
                                   0 };

int main()
{
  using namespace constants;

  std::srand(std::time(nullptr));
  wamp_router wr(REALM);

  auto wamp_router_th = std::thread([&]() {
      wr.run();
    });

  supervisor sv;
  game g(sv, wr.get_rs_port(), wr.get_uds_path());
  g.run();

  wr.shutdown();
  wamp_router_th.join();

  return 0;
}
  
void supervisor::set_ball_posture(std::array<double, 2>* posture)
{
  for(std::size_t i = 0; i < 2; ++i)
  {
    if (constants::BALL_INIT_RANDOM)
    {
      (*posture)[i] = (MAX_POSTURE[i] - MIN_POSTURE[i]) * std::rand() / RAND_MAX;
      (*posture)[i] += MIN_POSTURE[i];
    }
    else
    {
      (*posture)[i] = 0.0;
    }
  }
}

void supervisor::set_init_posture(bool is_red, std::size_t id, std::array<double, 3>* posture)
{
  const std::size_t active_players = is_red ? \
                                     constants::RED_TEAM_ACTIVE_PLAYER : \
                                     constants::BLUE_TEAM_ACTIVE_PLAYER;
  for(std::size_t i = 0; i < 3; ++i)
  {
    if (constants::ROBOT_INIT_RANDOM)
    {
      if (id < active_players)
      {
        (*posture)[i] = (MAX_POSTURE[i] - MIN_POSTURE[i]) * std::rand() / RAND_MAX;
        (*posture)[i] += MIN_POSTURE[i];
      }
      else
      {
        (*posture)[i] = constants::ROBOT_FOUL_POSTURE[id][i];
      }
    }
    else
    {
      (*posture)[i] = id < active_players ? \
                      constants::ROBOT_INIT_POSTURE[id][i] : \
                      constants::ROBOT_FOUL_POSTURE[id][i];
    }
  }
}
