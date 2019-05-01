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

bool check_distance(const std::array<double, 2>* a, const std::array<double, 2>* b) 
{
  // 5 for safety margin
  constexpr double min_distance_squared = 5 * constants::ROBOT_RADIUS * constants::ROBOT_RADIUS;
  double xdistance = (*a)[0] - (*b)[0];
  double ydistance = (*a)[1] - (*b)[1];
  return (xdistance * xdistance + ydistance * ydistance) > min_distance_squared;
}

bool check_distances(const std::array<double, 2>* a,
                     const std::vector<std::array<double, 2>>* preallocated_positions)
{
  for (const auto& b : *preallocated_positions) {
    if (!check_distance(a, &b)) return false;
  }
  return true;
}

double random_posture(std::size_t i)
{
  double posture = (MAX_POSTURE[i] - MIN_POSTURE[i]) * std::rand() / RAND_MAX;
  return posture + MIN_POSTURE[i];
}

void initialize_from_constants(const double* from, std::array<double, 3>* to, bool is_red) {
  for (std::size_t i = 0; i < 3; ++i) {
    (*to)[i] = from[i];
    if (!is_red) (*to)[i] *= -1.0;
  }
}
void supervisor::set_ball_posture(std::array<double, 2>* posture)
{
  for(std::size_t i = 0; i < 2; ++i)
  {
    if (constants::BALL_INIT_RANDOM) {
      (*posture)[i] = random_posture(i);
    } else {
      (*posture)[i] = 0.0;
    }
  }
}

void supervisor::set_init_posture(
    std::array<double, 2>* ball_posture,
    std::array<std::array<std::array<double, 3>, constants::NUMBER_OF_ROBOTS>, 2>* robot_posture)
{
  std::vector<std::array<double, 2>> preallocated_positions;
  preallocated_positions.push_back(*ball_posture);
  for (std::size_t team_id : {0, 1}) {
    for (std::size_t id = 0; id < constants::NUMBER_OF_ROBOTS; ++id) {
      if (constants::ACTIVENESS_[team_id][id]) {
        if (constants::ROBOT_INIT_RANDOM) {
          // random init + active player
          std::array<double, 2> posture;
          // pick a random posture.
          // if it overlapps with preallocated postures, repeat the process.
          do {
            posture[0] = random_posture(0);
            posture[1] = random_posture(1);
          } while (!check_distances(&posture, &preallocated_positions));
          preallocated_positions.push_back(posture);
          (*robot_posture)[team_id][id][0] = posture[0];
          (*robot_posture)[team_id][id][1] = posture[1];
          // theta has nothing to do with overlapping
          (*robot_posture)[team_id][id][2] = random_posture(2);
        } else {
          initialize_from_constants(constants::ROBOT_INIT_POSTURE[id],
                                    &(*robot_posture)[team_id][id],
                                    team_id == 0);
        }
      } else {
        initialize_from_constants(constants::ROBOT_FOUL_POSTURE[id],
                                  &(*robot_posture)[team_id][id],
                                  team_id == 0);
      }
    }
  }
}
