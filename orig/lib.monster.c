// ----------------------------------------------------------------------------
// monster.c (doc/build/monsters)
// Biko   - spells, abilities, limb-oriented combat...
// Midian - catch tell, intelligence...
// Trixx  - languages, abilities...
// Gloin  - catch tell, intelligence...
// Zor    - spells...
// Also: Greyson, many others.

// ----------------------------------------------------------------------------

#include "config.h"
#include "living.h" 
#include "levels.h"
#include "combat.h"
#include <daemons.h>

#define BALANCE
#undef BALANCE_LOGGING
#define PALDIN_NEW_BALANCE_CHECK 1

// ----------------------------------------------------------------------------

inherit "obj/basic/create";
inherit "obj/basic/id";
static inherit "lib/communication";
inherit "lib/living/living";
inherit "lib/player/basic/abilities";
inherit "lib/player/basic/lang";
inherit "lib/player/basic/msgs";
inherit "lib/monster/aggressive";
inherit "lib/monster/chat";
inherit "lib/monster/intelligence";
inherit "lib/monster/assist";

// ----------------------------------------------------------------------------

string short_desc;
string long_desc;
string alias;
string alt_name;
string unkillable;
int move_at_reset, aggressive;
status healing;
string a_chat_head;
int a_chat_chance;
status a_regular;
static int a_counter;
int random_pick;
static int counter, a_counter;
object kill_ob, dead_ob, init_ob;
object killer_ob; /* Added for monster_killer() (Ricochet 10-Jul-94 */
int *spell_chance, *spell_dam;
string *spell_type;
string *spell_msg_attacker, *spell_msg_others;
static int spell_size;
static status balanced;
#if PALDIN_NEW_BALANCE_CHECK
static int balanced_value;
#endif
static int num_resets;

object me;
object create_room;

// ----------------------------------------------------------------------------

id( str ) {
  return str==query_race() || id::id(str) || living::id(str) || 
    (query_race()=="human" &&
    ((query_gender()==1 && str == "man")||(query_gender()==2&&str=="woman")));
}

short() {
  return short_desc;
}

long() {
  writef( long_desc );
  if ( body_position )
    write( capitalize((string)this_object()->query_pronoun()) +
           " is currently "+query_body_position_string() +".\n" );
}

set_short( sh ) {
  short_desc = sh;
  if ( !long_desc )
    long_desc = short_desc + ".\n";
}

set_long( lo ) {
  long_desc = lo;
}

query_long() {
  return long_desc;
}

create() {
  string cname;

  ::create();
  cname = query_verb();
  if(cname == "clone" || cname == "Clone")
    log_file( "NPC_CLONE", "["+ ctime()[4..15] +"] "+ file_name(this_object()) +
      " by "+ this_interactive()->query_real_name() +"\n" );
  is_npc = 1;
  set_long("You see nothing special.\n");
  enable_commands();
 // Added Salliver 15 May, 1996 No need to call enable_languages(1) anymore!
  this_object()->set_ability("common language", 100);
  activate_languages();
 // End Salliver 15 May, 1996
  create_room = environment(me = this_object());

  set_head_str( "head" );
  set_body_str( "body" );
  set_arm_str( "arm" );
  set_leg_str( "leg" );
  set_hand_str( "hand" );
  set_foot_str( "foot" );

#ifdef BALANCE
  if ( is_clone( this_object() ) )
    call_out("check_balance", 1);
#else
   balanced = 1;
#endif
#if HALLOWEEN
  add_keyword("trick or treat", "do_trick_or_treat");
#endif
}

reset() { 
  ::reset();
  if(move_at_reset) random_move(); 
  if (num_resets<80) {
    num_resets++;
    experience+=(experience*20)/1000;
  }
}

init() {
  create_room = environment();
  if(this_player() == me) return;
  if(init_ob && init_ob->monster_init(me)) return;
  this_object()->assist_init();
  if(attacker_ob) set_heart_beat(1);
  if(this_player() && this_player()->query_is_player()) {
    set_heart_beat(1);
    if (query_aggressive(this_player())) {
      if (this_player()->query_level()>=WIZLEVEL)
        writef(capitalize(this_object()->short())+" looks at you "+
               "aggressively, but then realises that you are a wizard and "+
               "decides not to attack.");
      else
        kill_ob=this_player();
    }
  }
  if(!query_race()) set_race("human");
}

heart_beat() {
  int cur_spell;
  string *msgs;

  age += 1;
  if(!test_if_any_here()) {
    attack();
    set_heart_beat(0);
    if(!healing) heal_slowly();
    return;
  }
  if(kill_ob && !query_attack() && present(kill_ob, environment())) {
    if (random(2)) return; /* Delay attack some */
    attack_object(kill_ob);
    kill_ob = 0;
    return;
  }
#ifdef TESTMUD
  hook("heart_beat", ({ }) );
#endif
  if(!spell_size) cur_spell = -1;
  else if(spell_size == 1) cur_spell = 0;
  else cur_spell = random(spell_size);

  if(attacker_ob && cur_spell != -1 && present(attacker_ob, environment()) &&
     spell_chance[cur_spell] > random(100)) {
    string msg_attacker;
    msg_attacker = spell_msg_attacker[cur_spell];
    if ( ('#' == msg_attacker[0]) && ('#' == msg_attacker[1]) ) {
      call_other( this_object(), msg_attacker[2..] );
    } else {
      if(spell_msg_others[cur_spell] == "##") {
        msgs = get_message(msg_attacker,attacker_ob);
      } else {
        msgs = ({ msg_attacker,
          get_message(spell_msg_others[cur_spell], attacker_ob)[1] });
      }
      if((!spell_type[cur_spell]) || (spell_type[cur_spell] == "other")) {
        tell_object(attacker_ob, get_f_string(msgs[0]));
        say(get_f_string(msgs[1]), attacker_ob);
        attacker_ob->hit_player(random(spell_dam[cur_spell]), random(4) +1);
      } else {
        say(get_f_string(msgs[1]), attacker_ob);
        attacker_ob->special_hit(random(spell_dam[cur_spell]), random(4) +1,
          spell_type[cur_spell], "", msgs[0]);
      }
    }
  }
  attack();

  // if we were killed/destroyed, don't bug out during the rest of this
  if ( !this_object() )
    return;

#if 1
  if ( attacker_ob && whimpy && (hit_point < (1==whimpy?(max_hp/5):whimpy)) )
    run_away();
#else
  if(attacker_ob && whimpy && hit_point < max_hp / 5) run_away();
#endif

  // Determine if we should do an a_chat.
  if(a_chat_chance && attacker_ob && a_chat_head) {
      if(random(100) < a_chat_chance) {
        if(!a_regular) send_right_message(a_chat_head[random(sizeof(a_chat_head))], attacker_ob);
        else {
          if(a_counter>=sizeof(a_chat_head)) a_counter = 0;
          send_right_message(a_chat_head[a_counter],attacker_ob);
          a_counter++;
        }
      }
  } else {
    if(!attacker_ob) {
      // Might do a normal chat
      chat::heart_beat();
    }
  }
  if(random_pick && random(100) < random_pick) pick_any_obj();
}

pick_any_obj() {
  object ob;

  // the following added by Leoz 4-10-95
  if (!environment())
    return;
  
  for(ob = first_inventory(environment()); ob; ob = next_inventory(ob)) {
    if(ob->short() && ob->get()) {
      if(!add_weight(ob->query_weight())) {
        say(cap_name+ " tries to take " +ob->short()+ " but fails.\n");
        return;
      }
      say(cap_name+ " takes " +ob->short()+ ".\n");
      ob->move(this_object());
      if(ob) { //sanity check - Agarwaen 20151205
        if(ob->weapon_class()) ob->wield(ob->query_name());
        else if(ob->query_is_armour()) ob->wear(ob->query_name());
      }
      return;
    }
  }
}

heal_slowly() {
  hit_point += 120 / (INTERVAL_BETWEEN_HEALING * 2);
  if(hit_point > max_hp) hit_point = max_hp;
  spell_points += 120 / (INTERVAL_BETWEEN_HEALING * 2);
  if(spell_points > max_sp) spell_points = max_sp;
  healing = 1;
  if(hit_point < max_hp || spell_points < max_sp) call_out("heal_slowly", 120);
  else healing = 0;
}

tell_player( str ) {
  tell_object(this_object(), this_player()->query_name()+ " tells you: "
+str+"\n");
  return 1;
}

send_right_message( str, ob ) {
  string lan,what;

  if(str[0..1] == "##") {
    call_other(this_object(), str[2..strlen(str)]);
    return 1;
  }
  if(str[0] == '@') {
    if(sscanf(str,"@%s@%s",lan, what)!=2) return 1;
    language_say(what,lan);
    return 1;
  }
  if(ob) message(str, ob);
  else tell_room(environment(), str);
  return 1;
}

monster_died() {
}

is_target( ob ) {
  return (living(ob) && ob != this_object());
}

random_move() {
  command( ({ "north", "south", "east", "west" })[random( 4 )] );
}

second_life() {
  string file;
  file=explode(file_name(this_object()),"#")[0];
  "/w/piper/monsexp/reset_counter"->monster_killed(file,query_attack());
  if(dead_ob) return(dead_ob->monster_died(this_object())); 
}

init_command( cmd ) {
  command( cmd );
}

enable_languages() {
}

 /* MONSTER SETUP ********************************************************/

set_name( n ) {
  if(!id::set_name(n)) return;
  set_living_name(n);
  cap_name = capitalize(n);
  if (!short_desc)
    short_desc = cap_name;
}

set_level( l ) {
 //  Changed by Arpeggio 05/10/95: Enforce current Balance guidelines
 //  if they just call set_level

  int tmp,loop;
  int sk;

  if ( l < 1 )
    l = 1;
  else if ( l > 19 ) {
    // Next two lines added by Theryn for level 20+ monsters:
    if((int)"/d/Balance/secure/high_lvls"->high_lvl_approved(this_object()))
      balanced = 1;
    else l = 19;
  }

  level = tot_Str = Str = tmp_Str = tot_Dex = Dex = tmp_Dex = tot_Con = l;
  Con = tmp_Con = tot_Int = Int = tmp_Int = tot_Wis = Wis = tmp_Wis = l;
  restore_temp_stats();

//  weapon_class = l / 2 + 3;
  weapon_class = "d/Balance/secure/daemon"->query_balanced_npc_wc( l );
//  calculate_power();

  tmp = "d/Balance/secure/daemon"->query_balanced_npc_ac( l );
  head_ac = body_ac = arm_ac = leg_ac = tmp;

  tmp = 5*l + 5;
  for(sk=0;sk<sizeof(SKILL_TYPES);sk++) 
   set_skill(SKILL_TYPES[sk],tmp);

 // Changed by Paldin to remove Two Weapon as being a default skill for
 // All NPC's This can be set after level is called if it is required.
 // Changed 21-Feb-2004
 set_skill("Two Weapon",BALANCE_D->adjust_value(tmp,0,1077343865,1091318400));

  set_ability("parrying", l * 5);
  set_ability("blocking", l * 5);
  set_ability("magic resistance", l * 3);
  set_ability("fire resistance", l * 3);
  set_ability("cold resistance", l * 3);
  set_ability("poison resistance", l * 2);

//  max_hp = hit_point = spell_points = 50 + (level - 1) * 8;
  tmp = "d/Balance/secure/daemon"->query_balanced_npc_hp( l );
  max_hp = hit_point = spell_points = tmp;

  if(!(experience = "room/adv_guild"->query_cost(l-1))) experience = random(500);

//Following 28 May 2008 Sylwen to reduce exp for 20+ monsters
  if(l>19) experience = "/d/Balance/secure/high_lvls"->
                        query_highlvl_exp(l, experience);

  num_resets="/w/piper/monsexp/reset_counter"->query_last_kill(explode(file_name(this_object()),"#")[0]);
  if (num_resets!=0) {
    num_resets="/w/piper/monsexp/reset_counter"->query_current_reset()-num_resets;
    if (num_resets>55) {
      num_resets=55;
    }
    experience = (int)(experience * exp(num_resets * 0.0198)); // Add 2.0% per reset
  }  
}

set_armour_val( pos, val ) {
 int sk;
 for(sk = 0;sk<sizeof(DAMAGE_TYPES);sk++) {
  hit_armour[DAMAGE_TYPES[sk]][pos] = val;
 }
}

set_aim( i ) {
  aiming_at = i;
}

set_head_ac( ac ) {
  head_ac = ac;
}

set_body_ac( ac ) {
  body_ac = ac;
}

set_arm_ac( ac ) {
  arm_ac = ac;
}

set_leg_ac( ac ) {
  leg_ac = ac;
}

#if PALDIN_NEW_BALANCE_CHECK
set_balanced( i ) {
  balanced_value = i;
}
#endif

set_hp( hp ) {
  int balanced_hp;
  
  balanced_hp = "d/Balance/secure/daemon"->query_balanced_npc_hp( level );

  /* Arpeggio: This only logs hp changes after the monster has
     been created, and the check_balance call has fired */
  if ( hit_point && hit_point != hp && balanced ) {
    string changer;
    
    if ( this_player() ) {
      changer = this_player()->query_real_name();
      if ( !changer )
        changer = this_player()->query_name();
    }

    if ( !changer )
      changer = file_name( previous_object() );
    else
      changer = capitalize( changer );
    
    log_file( "HP_CHANGE", ctime()[4..15] +": "+ short() +"("+ file_name() +
      ") to "+ hp +"("+ balanced_hp +") by "+ changer +"\n" );
  }

  hit_point = max_hp = hp;
}

set_ep( ep ) {
  if(ep < experience) experience = ep;
}

set_al( al ) {
  alignment = al;
}

set_wc( wc ) {
  if (!wielded_weapon) weapon_class = wc;
  if (wielded_weapon && wc > weapon_class) weapon_class = wc;
//  calculate_power();
}

set_dex_wc( wc ) {
  dex_wc = wc;
  calculate_power();
}

set_move_at_reset() {
  move_at_reset = 1;
}

set_spell_type( t ) {
  spell_type = ({ t });
  spell_size = 1;
}

set_spell_chance( c ) {
  spell_chance = ({ c });
}

set_spell_msg_attacker( m ) {
  spell_msg_attacker = ({ m });
}

set_spell_msg_others( m ) {
  spell_msg_others = ({ m });
}

set_spell_dam( d ) {
  spell_dam = ({ d });
  if(!spell_size) spell_size = 1;
}

/* obsolete */ set_chance( c ) {
  spell_chance = ({ c });
}

/* obsolete */ set_spell_mess1( m ) {
  spell_msg_attacker = ({ m });
}

/* obsolete */ set_spell_mess2( m ) {
  spell_msg_others =  ({ m });
}

add_spell( arr ) {
  if(!arr || sizeof(arr) != 5) return;
  if(!spell_type) {
    spell_type = ({ arr[0] });
    spell_chance = ({ arr[1] });
    spell_dam = ({ arr[2] });
    spell_msg_attacker = ({ arr[3] });
    spell_msg_others = ({ arr[4] });
    spell_size = 1;
    return 1;
  }
  spell_type += ({ arr[0] });
  spell_chance += ({ arr[1] });
  spell_dam += ({ arr[2] });
  spell_msg_attacker += ({ arr[3] });
  spell_msg_others += ({ arr[4] });
  spell_size++;
  return 1;
}

set_frog() {
  frog = 1;
}

set_whimpy() {
  whimpy = 1;
}

void set_wimpy( int wimpy_level ) {
  whimpy = wimpy_level;
}

set_unkillable( str ) {
  unkillable = str;
}

set_dead_ob( ob ) {
  dead_ob = ob;
}

set_random_pick( r ) {
  random_pick = r;
}

set_init_ob( ob ) {
  init_ob = ob;
}

set_corpse( str ) {
  corpsefile = str;
}

set_stats( lstr, ldex, lint, lcon, lwis ) {
  if(lstr) set_str(lstr);
  if(ldex) set_dex(ldex);
  if(lint) set_int(lint);
  if(lcon) set_con(lcon);
  if(lwis) set_wis(lwis);
}

void set_a_chat_chance( int chance ) {
  a_chat_chance = chance;
}

load_a_chat( chance, strs, flag ) {
  sizeof(strs);
  a_chat_head = strs;
  a_chat_chance = chance;
  a_regular = flag;
}

query_a_chats() {
  return a_chat_head;
}

/************************************************************************/

query_corpse() {
  return corpsefile;
}

query_dead_ob() {
  return dead_ob;
}

query_real_name() {
  if ( query_name() ) return(lower_case(query_name()));
}

query_quests() {
  return "none";
}

query_unkillable() {
  return unkillable;
}

query_create_room() {
  return create_room;
}

query_name() {
  return living::query_name();
}

query_experience() {
  return experience;
}

query_spells() {
   if(!spell_size) return 0;
   return ({ spell_type, spell_dam, spell_chance });
}

nomask check_balance() {
  int AC, WC, HP;
#ifdef BALANCE_LOGGING
  string res;
#endif

// Removed by Paldin 9-Dec-2003 due to not required
//  if ( balanced ) return;
#if PALDIN_NEW_BALANCE_CHECK
  if ( balanced_value &&
      "/adm/usr/aisha/daemon"->query_balanced(this_object(),balanced_value)) {
      balanced = 1;
      return;
    }
#endif

#ifdef BALANCE_LOGGING
  res = "";
#endif

  AC = "d/Balance/secure/daemon"->query_balanced_npc_ac( level );
  if ( AC > 0 )
  {
    if ( body_ac != -1 && body_ac < AC )
    {
      body_ac = AC;
#ifdef BALANCE_LOGGING
      res += "Body ";
#endif
    }
    if ( arm_ac != -1 && arm_ac < AC )
    {
      arm_ac = AC;
#ifdef BALANCE_LOGGING
      res += "Arm ";
#endif
    }
    if ( leg_ac != -1 && leg_ac < AC )
    {
      leg_ac = AC;
#ifdef BALANCE_LOGGING
      res += "Leg ";
#endif
    }
    if ( head_ac != -1 && head_ac < AC )
    {
      head_ac = AC;
#ifdef BALANCE_LOGGING
      res += "Head ";
#endif
    }
  }
   
  WC = "d/Balance/secure/daemon"->query_balanced_npc_wc( level );
  if ( WC > 0 )
  {
    if ( weapon_class < WC )
    {
      weapon_class = WC;
#ifdef BALANCE_LOGGING
      res += "Weapon ";
#endif
    }
  }

  HP = "d/Balance/secure/daemon"->query_balanced_npc_hp( level );
  if ( HP > 0 )
  {
    if ( max_hp < HP )
    {
      max_hp = HP;
      hit_point = max_hp;
#ifdef BALANCE_LOGGING
      res += "Hitpoints ";
#endif
    }
  }

  balanced = 1;

#ifdef BALANCE_LOGGING
  if ( res != "" )
  {
    log_file( "BALANCE", ctime()[4..15] +": "+ file_name(this_object()) +
      "["+ geteuid(this_object()) +", "+ name +"] "+ res +"\n" );
  }
#endif
}

/* Following added for monster_killer() function. 10-Jul-94 by Ricochet */
set_killer_ob( ob ) {
  killer_ob = ob;
}

query_killer_ob() {
  return killer_ob;
}

monster_killer( ob ) {
}
 
add_exp( e ) {
  experience += e;
}

query_is_player() { return 0; }
int query_tameable() { return 1; }

int do_trick_or_treat() {
  if (catch(
    "/w/bytre/holiday/halloween/treat_daemon"->
      do_trick_or_treat(this_object(), this_player()) 
      )); 
  }

// ----------------------------------------------------------------------------
