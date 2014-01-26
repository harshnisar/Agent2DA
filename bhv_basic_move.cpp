// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bhv_basic_move.h"

#include "strategy.h"

#include "bhv_basic_tackle.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/body_intercept.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/player/intercept_table.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>

#include "neck_offensive_intercept_neck.h"

using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_BasicMove::execute( PlayerAgent * agent )
{
    dlog.addText( Logger::TEAM,
                  __FILE__": Bhv_BasicMove" );

    //-----------------------------------------------
    // tackle
    if ( Bhv_BasicTackle( 0.8, 80.0 ).execute( agent ) )
    {
        return true;
    }

    const WorldModel & wm = agent->world();
    /*--------------------------------------------------------*/
    // chase ball
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    if ( ! wm.existKickableTeammate()
         && ( self_min <= 3
              || ( self_min <= mate_min
                   && self_min < opp_min + 3 )
              )
         )
    {
        std::cout<<"Executing intercept, player - "<<wm.self().unum()<<std::endl;
        dlog.addText( Logger::TEAM,
                      __FILE__": intercept" );
        Body_Intercept().execute( agent );
        agent->setNeckAction( new Neck_OffensiveInterceptNeck() );

        return true;
    }

    const Vector2D target_point = Strategy::i().getPosition( wm.self().unum() );
    const double dash_power = Strategy::get_normal_dash_power( wm );

    double dist_thr = wm.ball().distFromSelf() * 0.1;
    if ( dist_thr < 1.0 ) dist_thr = 1.0;

    dlog.addText( Logger::TEAM,
                  __FILE__": Bhv_BasicMove target=(%.1f %.1f) dist_thr=%.2f",
                  target_point.x, target_point.y,
                  dist_thr );

    agent->debugClient().addMessage( "BasicMove%.0f", dash_power );
    agent->debugClient().setTarget( target_point );
    agent->debugClient().addCircle( target_point, dist_thr );

    if ( ! Body_GoToPoint( target_point, dist_thr, dash_power
                           ).execute( agent ) )
    {
        Body_TurnToBall().execute( agent );
    }

    if ( wm.existKickableOpponent()
         && wm.ball().distFromSelf() < 18.0 )
    {
        agent->setNeckAction( new Neck_TurnToBall() );
    }
    else
    {
        agent->setNeckAction( new Neck_TurnToBallOrScan() );
    }

    return true;
}

double 
Bhv_BasicMove::abs(double d){
    if (d>0.00)
        return d;
    else
        return d*(-1.00);
}

Vector2D 
Bhv_BasicMove::RoundToNearestTens(Vector2D P){
    // This method rounds a given position to its nearest tens - for example, the rounded position for (12, -17) would be (10, -20)
    // This helps in locating nearby holes more easily
    double multX = 10.00;
    double multY = 10.00;
    if(P.x<0.00)
        multX = -10.00;
    if(P.y<0.00)
        multY = -10.00;
    int roundX = static_cast<int>((abs(P.x)+5.00)/10);
    int roundY = static_cast<int>((abs(P.y)+5.00)/10);
    Vector2D roundedTens = Vector2D(multX*roundX, multY*roundY);
    //std::cout<<"Rounded Tens - "<<roundedTens<<std::endl;
    return roundedTens;
}

bool 
Bhv_BasicMove::isRTaHole(Vector2D P){
    // This method is only for rounded tens
    // Returns true iff rounded-ten point is a hole    
    int normalX = static_cast<int>(abs(P.x)/10);
    int normalY = static_cast<int>(abs(P.y)/10);
    if(normalX%2==normalY%2)
        return true;
    else
        return false;
}

Vector2D 
Bhv_BasicMove::RoundToNearestHole(Vector2D P){
    //std::cout<<"Rounding up point - "<<P<<std::endl;
    Vector2D roundedTens = RoundToNearestTens(P);
    if(isRTaHole(roundedTens)){
        //std::cout<<"RT is a hole - "<<roundedTens<<std::endl;
        return roundedTens;
    }
    else{
        Vector2D roundedHole;
        double diffX = P.x-roundedTens.x;
        double diffY = P.y-roundedTens.y;
            
        if(abs(diffX)<abs(diffY)){
            //Point closer to vertical axis of the diamond
            if(diffY>0)
                roundedHole = Vector2D(roundedTens.x, roundedTens.y+10);
            else
                roundedHole = Vector2D(roundedTens.x, roundedTens.y-10);
        }
        else{
            //Point closer to horizontal axis of the diamond
            if(diffX>0)
                roundedHole = Vector2D(roundedTens.x+10, roundedTens.y);
            else
                roundedHole = Vector2D(roundedTens.x-10, roundedTens.y);
        }
            //std::cout<<"Rounded hole - "<<roundedHole<<std::endl;
            return roundedHole;
    }
}
