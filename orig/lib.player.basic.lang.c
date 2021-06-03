// ----------------------------------------------------------------------------
// Language, by Salliver, Trixx, Dawg, and Arpeggio
// All races speak 'common language' and their own racial language.
// There are abilities associated with other languages that determine how well
// those languages are spoken and understood.

// ----------------------------------------------------------------------------

#pragma strict_types

#include <levels.h>

#define LANG            "secure/lang"
#define LOG(x)          log_file("LANGUAGE","basic/lang: "+x+"\n");
#define LAW_RECORD(x,y) "adm/usr/flint/lawlog"->write_law_log2(x,y)
// #define LINGUAL_WIZARDS

// ----------------------------------------------------------------------------

int query_npc();
int query_level();
string query_real_name();
int query_ability( string str );
int set_ability( string str, int num );
int remove_ability( string str );
static nomask void set_check_log( int i );

// ----------------------------------------------------------------------------

static string lang;   // actual language

// ----------------------------------------------------------------------------

string query_language() {
  return lang;
}

varargs int check_understand( string langu ) {
  if ( query_ability( langu ) >= 100 )
    return 1;

#ifdef LINGUAL_WIZARDS
  if ( query_level() > MAXMORTAL )
    return 1;
#endif

  return 0;
}

int change_language( string str ) {
  if ( !str ) {
    write( "You are speaking "+
      ((lang == "common language") ? "the common language.\n" : lang +".\n") );
    return 1;
  }

  str = lower_case( str );
  if ( str == "common" ) {
    str = "common language";
  } else if ( str == "cant" || str == "thieves' cant" ) {
    str = "Thieves' Cant";
  }

  if ( !LANG->query_exists( str ) ||
       (!query_npc() && (query_ability( str ) == -1)) ) {
    write( "You know of no such language.\n" );
    return 1;
  }

  if ( str == lang ) {
    write( "You are already speaking this language.\n" );
    return 1;
  }

  if ( !check_understand( str ) && !query_npc() ) {
    write( "You cannot speak that language!\n" );
    return 1;
  }
  
  write( "Changed language from "+ lang +" to "+ str +".\n" );
  lang = str;

  return 1;
} 

void understand_say( string who, string original, string message,
    string clang ) {
  if ( !query_npc() ) {
    set_check_log( 1 );
    if ( this_object()->query_lawlog() ) {
      LAW_RECORD( query_real_name(), get_f_string( who +" says: "+ original, 0,
        strlen( who ) + 7 ) );
    }
    set_check_log( 0 );
  }

  tell_object( this_object(), message );
}

void understand_whisper( string who, string original, string message,
    string clang ) {
  if ( !query_npc() ) {
    set_check_log( 1 );
    if ( this_object()->query_lawlog() ) {
      LAW_RECORD( query_real_name(), get_f_string( who +" whispers: "+ original,
        0, strlen( who ) + 11 ) );
    }
    set_check_log( 0 );
  }

  tell_object( this_object(), message );
}

varargs void understand_tell( string who, string original, string message,
    string clang, object who_ob ) {
  this_object()->add_to_thist( message,
    who_ob ? (interactive( who_ob ) ? "player" : "NPC") : 0, this_player()->query_real_name() );

  if ( interactive() && this_object()->query_misc( "tellbeep" ) &&
       this_object()->query_is_statue() )
    message = "\a"+ message;

  tell_object( this_object(), message );
}

int language_say( string str, string clang ) {
  if ( query_npc() || check_understand( clang ) )
    return (int)LANG->language_say( this_object(), str, clang );

  return 1;
}

int do_say( string str ) {
  return language_say( str, lang );
}

int language_sayto( string str, string clang, object target ) {
  if ( query_npc() || check_understand( clang ) )
    return (int)LANG->language_say( this_object(), str, clang, target );

  return 1;
}

int do_sayto( string str, object target ) {
  return language_sayto( str, lang, target );
}

int language_whisper( string str, string clang, object target ) {
  if ( query_npc() || check_understand( clang ) )
    return (int)LANG->language_whisper( this_object(), str, clang, target );

  return 1;
}

int do_whisper( string str, object target ) {
  return language_whisper( str, lang, target );
}

int language_tell( string str, string clang, object target ) {
  if ( query_npc() || check_understand( clang ) )
    return (int)LANG->language_tell( this_object(), str, clang, target );

  return 1;
}

int do_tell( string str, object target ) {
  return language_tell( str, lang, target );
}

void activate_languages() {
  if ( query_ability( "common language" ) == -1 )
    LANG->player_startup( this_object() );

  lang = "common language"; 

  add_action( "change_language", "language" );
}

varargs int add_language( string language, int ability, mixed arr,
    mixed extra ) {
  if ( arr ) {
    LOG( file_name() +" used old add_language interface." );

    // backwards compatibility
    if ( intp( extra ) ) {
      set_ability( language, extra );
    }

    return 0;
  }

  if ( !LANG->query_exists( language ) ) {
    LOG( file_name( previous_object() ) +" tried to add a bad language to "+
      query_real_name() );
    return 0;
  }

  set_ability( language, ability );
  return 1;
}

int modify_language() {
  LOG( file_name( this_object()) +" tried to modify language of "+
    query_real_name() );
  return 0;
}

int remove_language( string language, mixed extra ) {
  if ( !query_npc() && (("common language" == language) ||
        ("elvish" == language) || ("orcish" == language) ||
        ("dwarvish" == language)) ) {
    return 0;
  }

  if ( extra )
    LOG( file_name() +" tried to remove language using old interface." );
  
  remove_ability( language );
  if ( lang == language )
    change_language( "common language" );

  return 1;
}

// ----------------------------------------------------------------------------
