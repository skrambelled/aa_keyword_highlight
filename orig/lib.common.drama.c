// ----------------------------------------------------------------------------
// Drama

// ----------------------------------------------------------------------------

#include <drama.h>

#define DEBUG(x) tell_object(find_player("arpeggio"),x)

// ----------------------------------------------------------------------------

public varargs int drama( mixed *arr, int flags );

public int pause_drama();
public int continue_drama();

public int stop_drama();

public int player_left_drama();

public mixed *query_drama();

// ----------------------------------------------------------------------------

void do_drama();

// ----------------------------------------------------------------------------

/* private */ static mixed *the_drama;
private static int drama_flags;
private static int drama_step;
private static int drama_paused_delay;

// ----------------------------------------------------------------------------

public varargs int drama( mixed *arr, int flags ) {
  if ( !the_drama ) {
    if ( arr && pointerp( arr ) && sizeof( arr ) && intp( arr[0] ) ) {
      while ( -1 != remove_call_out( "do_drama" ) )
        ;

      the_drama   = arr;
      drama_flags = flags;
      drama_step  = 0;
      drama_paused_delay = -1;

      call_out( "do_drama", arr[0] );

      return 1;
    }
  }
}

public int pause_drama() {
  int x;

  x = remove_call_out( "do_drama" );
  if ( -1 != x ) {
    drama_paused_delay = x;
    return 1;
  }
}

public int continue_drama() {
  if ( -1 != drama_paused_delay ) {
    call_out( "do_drama", drama_paused_delay );

    drama_paused_delay = -1;
    return 1;
  }
}

public int stop_drama() {
  if ( the_drama ) {
    while ( -1 != remove_call_out( "do_drama" ) )
      ;

    drama_step  = 0;
    drama_paused_delay = -1;
    drama_flags = 0;
    the_drama   = 0;

    return 1;
  }
}

public mixed *query_drama() {
  return the_drama;
}

public int player_left_drama() {
  return 0;
}

void do_drama() {
  if ( the_drama ) {
    mixed *my_drama;
    my_drama = the_drama;

    ++drama_step;

    if ( (drama_step >= 0) && (drama_step < sizeof( the_drama )) ) {
      int continue_drama;
      continue_drama = 1;

      if ( drama_flags & DRAMA_FLAGS_PLAYER_LEFT ) {
        if ( !this_player() ) {
          continue_drama = player_left_drama();
        } else {
          if ( this_object()->query_room() ) {
            if ( environment( this_player() ) != this_object() ) {
              continue_drama = player_left_drama();
            }
          } else {
            if ( (this_player() == this_object()) ||
                 (environment(this_player()) != environment(this_object())) ) {
              continue_drama = player_left_drama();
            }
          }
        }
      }

      if ( continue_drama ) {
        if ( stringp( the_drama[drama_step] ) ) {
          string str;
          str = the_drama[drama_step];

          if ( ('#' == str[0]) && ('#' == str[1]) ) {
            call_other( this_object(), str[2..] );
          } else {
            this_object()->message( str, this_player() );
          }
        } else if ( pointerp( the_drama[drama_step] ) ) {
          mixed *action;
          action = the_drama[drama_step];

          if ( sizeof( action ) ) {
            switch ( action[0] ) {
            case DRAMA_TELL_ROOM:
              if ( 5 <= sizeof( action ) ) {
                if ( action[1] && stringp( action[4] ) ) {
                  if ( DRAMA_FORMAT_NONE == action[3] ) {
                    if ( action[2] ) {
                      tell_room( action[1], action[4], action[2] );
                    } else {
                      tell_room( action[1], action[4] );
                    }
                  } else if ( DRAMA_FORMAT_WRAP_INDENT == action[3] ) {
                    if ( 7 == sizeof( action ) ) {
                      if ( action[2] ) {
                        tell_room( action[1], get_f_string( action[4],
                          action[5], action[6] ), action[2] );
                      } else {
                        tell_room( action[1], get_f_string( action[4],
                          action[5], action[6] ) );
                      }
                    }
                  }
                }
              }
              break;
            case DRAMA_TELL_OBJECT:
              if ( 4 <= sizeof( action ) ) {
                if ( action[1] && stringp( action[3] ) ) {
                  if ( DRAMA_FORMAT_NONE == action[2] ) {
                    tell_object( action[1], action[3] );
                  } else if ( DRAMA_FORMAT_WRAP_INDENT == action[2] ) {
                    if ( 6 == sizeof( action ) ) {
                      tell_object( action[1], get_f_string( action[3],
                        action[4], action[5] ) );
                    }
                  }
                }
              }
              break;
            case DRAMA_TELL:
              if ( 3 <= sizeof( action ) ) {
                if ( action[1] && stringp( action[2] ) ) {
                  if ( 4 == sizeof( action ) ) {
                    this_object()->language_tell( action[2], action[3],
                      action[1] );
                  } else {
                    this_object()->do_tell( action[2], action[1] );
                  }
                }
              }
              break;
            case DRAMA_SAY:
              if ( 2 <= sizeof( action ) ) {
                if ( stringp( action[1] ) ) {
                  if ( 3 == sizeof( action ) ) {
                    this_object()->language_say( action[1], action[2] );
                  } else {
                    this_object()->do_say( action[1] );
                  }
                }
              }
              break;
            case DRAMA_SAYTO:
              if ( 3 <= sizeof( action ) ) {
                if ( action[1] && stringp( action[2] ) ) {
                  if ( 4 == sizeof( action ) ) {
                    this_object()->language_sayto( action[2], action[3],
                      action[1] );
                  } else {
                    this_object()->do_sayto( action[2], action[1] );
                  }
                }
              }
              break;
            case DRAMA_WHISPER:
              if ( 3 <= sizeof( action ) ) {
                if ( action[1] && stringp( action[2] ) ) {
                  if ( 4 == sizeof( action ) ) {
                    this_object()->language_whisper( action[2], action[3],
                      action[1] );
                  } else {
                    this_object()->do_whisper( action[2], action[1] );
                  }
                }
              }
              break;
            case DRAMA_MESSAGE:
              if ( 3 == sizeof( action ) ) {
                if ( action[1] && stringp( action[2] ) ) {
                  this_object()->message( action[2], action[1] );
                }
              }
              break;
            case DRAMA_FUNCTION:
              if ( 3 <= sizeof( action ) ) {
                if ( action[1] && stringp( action[2] ) ) {
                  if ( 4 == sizeof( action ) ) {
                    call_other( action[1], action[2], action[3] );
                  } else {
                    call_other( action[1], action[2] );
                  }
                }
              }
              break;
            }
          }
        }
      }

      if ( continue_drama ) { 
        if ( the_drama && (the_drama == my_drama) ) {
          ++drama_step;
          if ( drama_step < sizeof( the_drama ) ) {
            call_out( "do_drama", the_drama[drama_step] );
          } else {
            stop_drama();
          }
        }
      } else {
        stop_drama();
      }
    } else {
      stop_drama();
    }
  }
}

// ----------------------------------------------------------------------------
