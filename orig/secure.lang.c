// This object contains all the language 'sounds' and implements
// all of the text enciphering routines

// -------------------------------------------------------------- PREPROCESSOR

#pragma strict_types

#include <levels.h>

// Macro for efficiency - IS_ALPHA(x) is 1 if x is a lower case char
// 2 if it is upper case, and 0 if it is not an alphabetical char.

#define IS_ALPHA(c)   (c > 96 && c < 123?1:(c > 64 && c < 91?2:0))
#define LANG_SAVEFILE "/obj/save/lang"
#define LOG(x)        log_file("LANGUAGE","secure/lang: "+x+"\n");

// ----------------------------------------------------------------- VARIABLES

private mapping lang_map;  // the map with the language data 

// ------------------------------------------------------------ IMPLEMENTATION

void create() {
  seteuid( getuid() );
  if ( is_clone() ) {
    raise_error( "* Do not clone this object\n" );
    destruct( this_object() );
  } else {
    restore_object( LANG_SAVEFILE );
    if ( !lang_map ) {
      lang_map = ([ "elvish":
        ({ "gl","el","en","f","h","i","la","n","r","s","va","w","y" }),
        "dwarvish":
        ({ "a","b","d","kh","m","n","oi","ori","th","u","vo","z" }),
        "orcish":
        ({ "a","gh","gr","k","m","p","r","sh","sk","t","uu","x","z" }),
        "common language":1,
        "cityspeak":
        ({ "b","i","t","e","m","e" }),
        "Thieves' Cant":
        ({ "b","i","t","e","m","e" })
      ]);

//    save_object( LANG_SAVEFILE );
    }
  }
}

int query_exists( string str ) {
  return member( lang_map, str );
}

// these are global to cut down on inefficiency of parameter passing
// Note that I hate using globals (this was quite hard to debug) but
// efficiency dictates their use in this case.
// These functions are all private because they must be called in 
// the correct order or their results will be unknown.

private static object current;
private static string *cleartext,*ciphertext, clang, name, message;
private static int total_words, *word_size_order, *lengths;

private int size_sort( int a, int b ) {
  return (lengths[a] > lengths[b]);
}

// The original lang.c algorithm has been rewritten (for much improved
// efficiency).

// A language string is enciphered by breaking it up into separate
// words, and then producing an array containg a representation of the
// word based on enciphering each character in the word with a string 
// randomly chosen from the language's "sound", which is stored in the
// language map.
// Another array, word_size_order (wso), is built up that looks like this: 
// wso[0] = 0, wso[1] = 1, ... ,wso[sizeof(cleartext)] = sizeof(cleartext).
// This array is then shuffled by sorting it based on the lengths of
// the word at that position so the result is an array that shows the
// position of the shortest to longest (cleartext) word in the cleartext 
// and ciphertext arrays.  From this array it is a simple task to build
// up a string that each object "hears", based on the number of
// words in the string that it understands.

private void lang_encipher() {
  int i,j,size;
  string *mumbo;

  mumbo = lang_map[clang];
  size = sizeof( mumbo );
  total_words = sizeof( cleartext );
  lengths = allocate( total_words );
  ciphertext = allocate( total_words );
  for ( i = 0, word_size_order = allocate( total_words ) ;
        i < total_words ; ++i ) {
    lengths[i] = strlen( cleartext[i] );
    word_size_order[i] = i;
    ciphertext[i] = "";
    for ( j = 0 ; j < lengths[i] ; ++j ) {
      switch ( cleartext[i][j] ) {
      case 'A'..'Z':
        ciphertext[i] += capitalize( mumbo[random( size )] );
        break;
      case 'a'..'z':
        ciphertext[i] += mumbo[random( size )];
        break;
      default:
        ciphertext[i] += extract( cleartext[i], j, j );
        break;
      }
    }
  }

  word_size_order = sort_array( word_size_order, #'size_sort );
}

// understanding calculates how many words an object will understand
// using the old calculation from the original lang.c.
// It then steps through the array which shows the position of the shortest
// to longest word, placing unenciphered text at the appropriate positions
// in the string until it has done so for the number of words that will be
// understood.  Next, it fills in the remaining word positions with 
// their enciphered equivalents.  This results in the object understanding
// the shortest words and not understanding the longest words.

private string understanding( int ability ) {
  int words,i;
  string *rcvd;

  rcvd = allocate( total_words );
  words = (total_words * ability + 50 + random( 201 ) - 100) / 100;
  if ( words > total_words )
    words = total_words;

  for ( i = 0 ; i < words ; ++i )
    rcvd[word_size_order[i]] = cleartext[word_size_order[i]];
  for ( ; i < total_words ; ++i )
    rcvd[word_size_order[i]] = ciphertext[word_size_order[i]];

  return implode( rcvd, " " );
}

private void understand_say( object pl, string good_p, string bad_p ) {
  if ( !pl->check_understand( clang ) ) {
    pl->understand_say( name, message, get_f_string( bad_p +
      understanding( (int)pl->query_ability( clang ) ),
      0, strlen( bad_p ) ), clang );
  } else {
    pl->understand_say( name, message, get_f_string( good_p + message,
      0, strlen( good_p ) ), clang );
  }
}

private int is_target( object pl ) {
  return (living( pl ) && (pl != current));
}

// People on World, Mudlib, Senators and Arches have access to 
// change languages.
private nomask int lang_access() {
  string name;

  if ( !this_interactive() || (this_interactive() != this_player()) )
    return 0;

  name = (string)this_player()->query_real_name();
  // Check if they are in mudlib.
  if ( member( explode( read_file( "/d/Mudlib/MEMBERS" ),"\n" ), name ) >= 0 )
    return 1;
  if ( member( explode( read_file( "/d/World/MEMBERS" ), "\n" ), name ) >=0 )
    return 1;
  if ( (int)this_interactive()->query_level() >= SENATELEVEL )
    return 1;

  return 0;
}

varargs public int language_say( object pl, string str, string lang,
    object target ) {
  object en;
  string target_name;

  if ( !(en = environment( pl )) || !member( lang_map, lang ) )
    return 0;

  current = pl;
  clang = lang;
  if ( pl->query_npc() && !pl->query_name_say() ) {
    name = capitalize( (string)pl->short() );
    if ( strlen( name ) > 30 )
      name = (string)pl->query_name();
  } else {
    name = (string)pl->query_name();
  }
  if ( target ) {
    if ( target->query_npc() && !target->query_name_say() ) {
      target_name = capitalize( (string)target->short() );
      if ( strlen( target_name ) > 30 )
        target_name = (string)target->query_name();
    } else {
      target_name = (string)target->query_name();
    }
  }

  if ( clang == "common language" ) {
    object ob;
    string message_target;

    switch ( str[<1] ) {
      case '!':
        if ( target ) {
          message = get_f_string( name +" exclaims to "+ target_name +": "+ str,
            0, strlen( name ) + strlen( target_name ) + 15 );
          message_target = get_f_string( name +" exclaims to you: "+ str, 0,
            strlen( name ) + 18 );
        } else {
          message = get_f_string( name +" exclaims: "+ str, 0,
            strlen( name ) + 11 );
        }
        break;
      case '?':
        if ( target ) {
          message = get_f_string( name +" asks "+ target_name +": "+ str, 0,
            strlen( name ) + strlen( target_name ) + 8 );
          message_target = get_f_string( name +" asks you: "+ str, 0,
            strlen( name ) + 11 );
        } else {
          message = get_f_string( name +" asks: "+ str, 0,
            strlen( name ) + 7 );
        }
        break;
      default:
        if ( target ) {
          message = get_f_string( name +" says to "+ target_name +": "+ str, 0,
            strlen( name ) + strlen( target_name ) + 11 );
          message_target = get_f_string( name +" says to you: "+ str, 0,
            strlen( name ) + 14 );
        } else {
          message = get_f_string( name +" says: "+ str, 0,
            strlen( name ) + 7 );
        }
        break;
    }
    
    for ( ob = first_inventory( en ) ; ob ; ob = next_inventory( ob ) ) {
      if ( living( ob ) && (ob != pl) ) {
        if ( ob == target ) {
          ob->understand_say( name, str, message_target, clang );
        } else {
          ob->understand_say( name, str, message, clang );
        }
      }
    }
//    filter_array( filter_array( all_inventory( en ), #'is_target ),
//      #'call_other, "understand_say", name, str, message, clang );
    en->understand_say( name, str, message, clang );
  } else {
    string good_p, bad_p, target_p;
    int    good_p_len, bad_p_len;

    cleartext = explode( str +" ", " " );
    message = str;
    lang_encipher();

    switch ( str[<1] ) {
      case '!': {
        if ( target ) {
          if ( target->check_understand( clang ) ) {
            target_p = name +" exclaims to you in "+ clang +": ";
          } else {
            target_p = name +" seems to exclaim to you: ";
          }
          good_p = name +" exclaims to "+ target_name +" in "+ clang +": ";
          bad_p = name +" seems to exclaim to "+ target_name +": ";
        } else {
          good_p = name +" exclaims in "+ clang +": ";
          bad_p = name +" seems to exclaim: ";
        }
        break;
      }
      case '?': {
        if ( target ) {
          if ( target->check_understand( clang ) ) {
            target_p = name +" asks you in "+ clang +": ";
          } else {
            target_p = name +" seems to ask you: ";
          }
          good_p = name +" asks "+ target_name +" in "+ clang +": ";
          bad_p = name +" seems to ask "+ target_name +": ";
        } else {
          good_p = name +" asks in "+ clang +": ";
          bad_p = name +" seems to ask: ";
        }
        break;
      }
      default: {
        if ( target ) {
          if ( target->check_understand( clang ) ) {
            target_p = name +" says to you in "+ clang +": ";
          } else {
            target_p = name +" seems to say to you: ";
          }
          good_p = name +" says to "+ target_name +" in "+ clang +": ";
          bad_p = name +" seems to say to "+ target_name +": ";
        } else {
          good_p = name +" says in "+ clang +": ";
          bad_p = name +" seems to say: ";
        }
        break;
      }
    }
    good_p_len = strlen( good_p );
    bad_p_len = strlen( bad_p );

    for ( ob = first_inventory( en ) ; ob ; ob = next_inventory( ob ) ) {
      if ( living( ob ) && (ob != pl) ) {
        if ( ob == target ) {
          if ( ob->check_understand( clang ) ) {
            ob->understand_say( name, str, get_f_string( target_p + str,
              0, strlen( target_p ) ) );
          } else {
            ob->understand_say( name, str, get_f_string( target_p +
              understanding( (int)ob->query_ability( clang ) ),
              0, strlen( target_p ) ) );
          }
        } else {
          if ( ob->check_understand( clang ) ) {
            ob->understand_say( name, str, get_f_string( good_p + str,
              0, good_p_len ) );
          } else {
            ob->understand_say( name, str, get_f_string( bad_p +
              understanding( (int)ob->query_ability( clang ) ),
              0, bad_p_len ) );
          }
        }
      }
    }
//    filter_array( filter_array( all_inventory( en ), #'is_target ),
//      #'understand_say, good_p, bad_p );
    understand_say( en, good_p, bad_p );
  }

  return 1;
}

int language_whisper( object pl, string str, string lang, object target ) {
  if ( !member( lang_map, lang ) )
    return 0;

  current = pl;
  if ( (pl->query_invis()) && (MAXMORTAL < (int)target->query_level()) ) {
    name = "("+ capitalize( (string)pl->query_real_name() ) +")";
  } else {
    name = (string)pl->query_name();
  }

  if ( lang == "common language" ) {
    target->understand_whisper( name, str, get_f_string( name +
      " whispers to you: "+ str, 0, strlen( name ) + 18 ), lang );
  } else {
    clang = lang;
    cleartext = explode( str +" ", " " );
    message = str;
    lang_encipher();

    if ( target->check_understand( clang ) ) {
      target->understand_whisper( name, str, get_f_string( name +
        " whispers to you in "+ clang +": "+ message, 0,
        strlen( name ) + strlen( clang ) + 22 ), clang );
    } else {
      target->understand_whisper( name, str, get_f_string( name +
        " seems to whisper to you: "+
        understanding( (int)target->query_ability( clang ) ), 0,
        strlen( name ) + 26 ), clang ); 
    }
  }

  return 1;
}

int language_tell( object pl, string str, string lang, object target ) {
  if ( !member( lang_map, lang ) )
    return 0;

  current = pl;
  if ( (pl->query_invis()) && (MAXMORTAL < (int)target->query_level()) ) {
    name = "("+ capitalize( (string)pl->query_real_name() ) +")";
  } else {
    name = (string)pl->query_name();
  }

  if ( lang == "common language" ) {
    target->understand_tell( name, str, get_f_string( name +
      " tells you: "+ str, 0, strlen( name ) + 12 ), lang, pl );
  } else {
    clang = lang;
    cleartext = explode( str +" ", " " );
    message = str;
    lang_encipher();

    if ( target->check_understand( clang ) ) {
      target->understand_tell( name, str, get_f_string( name +
        " tells you in "+ clang +": "+ message, 0,
        strlen( name ) + strlen( clang ) + 14 ), clang, pl );
    } else {
      target->understand_tell( name, str, get_f_string( name +
        " seems to tell you: "+
        understanding( (int)target->query_ability( clang ) ), 0,
        strlen( name ) + 20 ), clang, pl ); 
    }
  }

  return 1;
}

// This interface will encipher text based on pl's understanding of
// the language.  It returns a two dimensional array (val, say) on success,
// with val[0] being the text that the user will see, and val[1] is
// 0 if the player doesn't fully understand the language, or 1 if they do.
// if an error occurs, 0 is returned

mixed *encipher_text( object pl, string text, string language ) {
  string *lines, *line_words;
  int *len;
  int i,j, nr_lines,counter;

  if ( !objectp( pl ) || !interactive( pl ) )
    return 0;

  // Everyone understands common.
  if ( language == "common" )
    return ({ text, 1 });
  if ( !member( lang_map, language ) )
    return 0;
  if ( pl->check_understand( language ) )
    return ({ text, 1 });

  clang = language;
  lines = explode( text +"\n", "\n" );
  nr_lines = sizeof( lines );
  cleartext = ({});
  for ( len = allocate( nr_lines + 1 ) , len[0] = 0 ;
        i < nr_lines ; ++i ) {
    line_words = explode( lines[i] +" ", " " );
    cleartext += line_words;
    len[i+1] = sizeof( line_words ) + len[i];
  }
  lang_encipher();
  line_words = explode( understanding( (int)pl->query_ability( language ) ) +
    " ", " " );
  for ( i = 0 ; i < nr_lines ; ++i )
    lines[i] = implode( line_words[len[i]..len[i+1]-1], " " );

  return ({ implode( lines, "\n" ), 0 });
}

///////////////////////////////////////////////////////
//      routines to add/modify/remove languages      //
///////////////////////////////////////////////////////

int add_language( string language, string *cipher ) {
  if ( !lang_access() )
    return 0;
  if ( lang_map[language] ) {
    return 0;
  } else {
    LOG( (string)this_player()->query_real_name() +" added "+ language );
    lang_map += ([ language : cipher ]);
    save_object( LANG_SAVEFILE );
    return 1;
  }
}

int modify_language( string language, string *cipher ) {
  if ( !lang_access() )
    return 0;
  if ( !member( lang_map, language ) ) {
    return 0;
  } else {
    LOG( (string)this_player()->query_real_name() +" modified "+ language );
    lang_map[language] = cipher;
    save_object( LANG_SAVEFILE );
    return 1;
  }
}

int remove_language( string language ) {
  if ( !lang_access() )
    return 0;
  if ( member( ({ "common language", "elvish", "orcish", "dwarvish" }),
         language ) != -1 || !member( lang_map, language ) ) {
    return 0;
  } else {
    LOG( (string)this_player()->query_real_name() +" removed "+ language );
    lang_map -= ([ language ]);
    save_object( LANG_SAVEFILE );
    return 1;
  }
}

//////////////////////////////////////////////////////////
// This function is only ever called from a player      //
// character object when the character is first created //
//////////////////////////////////////////////////////////

void player_startup( object pl ) {
  mapping startup_map;
  string race;

  if ( pl != previous_object() ) {
    LOG( (string)previous_object()->query_real_name() +" tried to reactivate "+
      (string)pl->query_real_name() +"'s language abilities." );
    return;
  }

  pl->remove_ability( "language" );
  pl->set_ability( "common language", 100 );
  race = (string) pl->query_race();
  if ( member( ({ "human","elf","half-elf","orc","dwarf" }), race ) == -1 )
    return;

  startup_map = ([ "human"   :({ random(20)+10,random(20)+10,random(20)+10}),
    	           "elf"     :({ 100,random(10),random(20)}),
    	           "half-elf":({ 100,random(10),random(20)}),
    	           "orc"     :({ random(10),100,random(20)}),
    	           "dwarf"   :({ random(20),random(20),100}) ]);

  pl->set_ability( "elvish", startup_map[race][0] );
  pl->set_ability( "orcish", startup_map[race][1] );
  pl->set_ability( "dwarvish", startup_map[race][2] );
}

string race_language( string race ) {
  if ( race ) {
    switch( lower_case( race ) ) {
      case "elf":   return "elvish";
      case "dwarf": return "dwarvish";
      case "orc":   return "orcish";
      default:      return "common";
    }
  }
}

// ----------------------------------------------------------------------- EOF
