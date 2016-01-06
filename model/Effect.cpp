#include "Effect.h"
#include <sstream>
#include <algorithm>
#include "scen.h"
#include "TriggerVisitor.h"

#include "../view/utilunit.h"
#include "../util/utilio.h"
#include "../view/utilui.h"
#include "../util/Buffer.h"
#include "../util/helper.h"
#include "../util/settings.h"
#include "../util/cpp11compat.h"

using std::vector;

extern class Scenario scen;

static const long MAXFIELDS=24;

const long Effect::defaultvals[MAXFIELDS] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

Effect::Effect()
:	ECBase(EFFECT),
	pUnit(NULL),
	pTech(NULL),
	valid_since_last_check(true)
{
    size = MAXFIELDS;
    memcpy(&ai_goal, defaultvals, sizeof(defaultvals));
	memset(uids, -1, sizeof(uids));
}

// Can only delegate (chain) constructors since C++11
Effect::Effect(Buffer &b)
:	ECBase(EFFECT),
	pUnit(NULL),
	pTech(NULL),
	valid_since_last_check(true)
{
    memcpy(&ai_goal, defaultvals, sizeof(defaultvals));
    memset(uids, -1, sizeof(uids));

    ClipboardType::Value cliptype;
    b.read(&cliptype, sizeof(cliptype));
    if (cliptype != ClipboardType::EFFECT)
        throw bad_data_error("Effect has incorrect check value.");

    b.read(&type, sizeof(type));
    b.read(&size, sizeof(size));

    if (size >= 0) {
        long * flatdata = new long[size];
        b.read(flatdata, sizeof(long) * size);
        memcpy(&ai_goal, flatdata, sizeof(long) * (size<=MAXFIELDS?size:MAXFIELDS));
        delete[] flatdata;
    }

    if (size >= 7)
        pUnit = esdata.units.getByIdSafe(unit_cnst);
    if (size >= 10)
	    pTech = esdata.techs.getByIdSafe(tech_cnst);

	// non-flat data
	text.read(b, sizeof(long));
	sound.read(b, sizeof(long));
	if (num_sel > 0)
		b.read(uids, sizeof(uids));

	check_and_save();
}

void Effect::compress()
{
    if (pUnit)
        unit_cnst = pUnit->id();
    if (pTech)
        tech_cnst = pTech->id();

    size = MAXFIELDS;
    for (int i = size - 1; i >= 0; i--) {
        if (*(&ai_goal + i) == defaultvals[i]) {
            size--;
        } else {
            if (setts.intense)
                printf_log("effect size %d.\n",size);
            break;
        }
    }
}

/* Used forBuffer operations (i.e., copy & paste) */
void Effect::tobuffer(Buffer &b)// const (make it const when unit_cnst gets set elsewhere)
{
    ClipboardType::Value cliptype = ClipboardType::EFFECT;
	b.write(&cliptype, sizeof(cliptype));

	b.write(&type, sizeof(type));
	b.write(&size, sizeof(size));

    if (size >= 7)
        unit_cnst = pUnit ? pUnit->id() : -1;
    if (size >= 10)
	    tech_cnst = pTech ? pTech->id() : -1;

    if (size >= 0 && size <= MAXFIELDS) {
	    b.write(&ai_goal, sizeof(long) * (size<=MAXFIELDS?size:MAXFIELDS));
    }

	text.write(b, sizeof(long));
	sound.write(b, sizeof(long));
	if (num_sel > 0)
		b.write(uids, sizeof(uids));
}

void Effect::read(FILE *in)
{
    memcpy(&ai_goal, defaultvals, sizeof(defaultvals));
    memset(uids, -1, sizeof(uids));

    readbin(in, &type);
    readbin(in, &size);

    if (size < 0 || size > MAXFIELDS)
        throw bad_data_error("Effect has incorrect size value.");

	size_t read = fread(&ai_goal, sizeof(long), size, in);
	if (read != (size_t)size)
	{
		throw bad_data_error(
			(feof(in)) ? "Early EOF" : "stream error");
	}

    if (size >= 7)
        pUnit = esdata.units.getByIdSafe(unit_cnst);
    if (size >= 10)
	    pTech = esdata.techs.getByIdSafe(tech_cnst);

	text.read(in, sizeof(long));
	sound.read(in, sizeof(long));

    // this helped me find why there was heap corruption
    // && num_sel < 22
    // Need a bigger number for max selected units
	if (num_sel > 0)
		readbin(in, uids, num_sel);

    // DODGY! should be removed as soon as bug is fixed
	if ((long)trig_index < 0) {
	    trig_index = -1;
	}

	check_and_save();
}

void Effect::write(FILE *out)
{
    compress();

	writebin(out, &type);
	writebin(out, &size);

    if (size >= 7)
        unit_cnst = pUnit ? pUnit->id() : -1;
    if (size >= 10)
	    tech_cnst = pTech ? pTech->id() : -1;

    if (size >= 0 && size <= MAXFIELDS) {
	    fwrite(&ai_goal, sizeof(long) * size, 1, out);
    }

	text.write(out, sizeof(long), true);
	sound.write(out, sizeof(long), true);
	if (num_sel > 0)
		fwrite(uids, sizeof(long), num_sel, out);
}

std::string pluralizeUnitType(std::string unit_type_name) {
    std::ostringstream convert;
    replaced(unit_type_name, ", Unpacked", "");
    if (!unit_type_name.empty() && *unit_type_name.rbegin() != 's' && !replaced(unit_type_name, "man", "men")) {
        convert << unit_type_name << "s";
    } else {
        convert << unit_type_name;
    }
    return convert.str();
}

std::string Effect::areaName() const {
    std::ostringstream convert;
    if (valid_area()) {
        if (valid_partial_map()) {
            if (valid_area_location()) {
                convert << " at " << area.left << "," << area.top;
            } else {
                convert << " in area " << area.left << "," << area.bottom << " [" << area.right - area.left + 1 << "x" << area.top - area.bottom + 1 << "]";
            }
        }
    } else {
        convert << " in INVALID AREA";
    }
    return convert.str();
}

inline std::string unitTypeName(const UnitLink *pUnit) {
    std::ostringstream convert;
    if (pUnit && pUnit->id()) {
        std::wstring unitname(pUnit->name());
        std::string un(unitname.begin(), unitname.end());
        convert << un;
    } else {
        convert << "unit";
    }
    return convert.str();
}

std::string Effect::selectedUnits() const {
    std::ostringstream convert;
    bool class_selected = group >= 0 && group < NUM_GROUPS - 1;
    bool unit_type_selected = utype >= 0 && utype < NUM_UTYPES - 1;
    if (s_player >= 0)
        convert << playerPronoun(s_player) << " ";
    if (pUnit && pUnit->id()) {
        if (num_sel > 1) {
            convert << pluralizeUnitType(unitTypeName(pUnit));
        } else {
            convert << unitTypeName(pUnit);
        }
    } else if (class_selected || unit_type_selected) { // account for groups[0]={-1,None}
        if (class_selected && unit_type_selected) {
            convert << "units of both class " << groups[group + 1].name;
            convert << " and type " << utypes[utype + 1].name;
        } else if (class_selected) {
            convert << "units of type " << groups[group + 1].name;
        } else {
            convert << "units of class " << utypes[utype + 1].name;
        }
    } else {
        convert << "units";
    }
	for (int i = 0; i < num_sel; i++) {
        convert << " " << uids[i] << " (" << get_unit_full_name(uids[i]) << ")";
	}
    convert << areaName();
    return convert.str();
}

// functor used for getting a subset of an array of PlayerUnits
class playersunit_ucnst_equals
{
public:
	playersunit_ucnst_equals(UCNST cnst)
		: _cnst(cnst)
	{
	}

	bool operator()(const PlayersUnit& pu)
	{
	    bool sametype = false;
        if (pu.u) {
            const UnitLink * ul = pu.u->getType();
	        if (ul->id() == _cnst) {
	            sametype = true;
	        }
        }
		return (sametype);
	}

private:
	UCNST _cnst;
};

// functor used for getting a subset of an array of PlayerUnits
class playersunit_ucnst_notequals
{
public:
	playersunit_ucnst_notequals(UCNST cnst)
		: _cnst(cnst)
	{
	}

	bool operator()(const PlayersUnit& pu)
	{
	    bool difftype = false;
        if (pu.u) {
            const UnitLink * ul = pu.u->getType();
	        if (ul->id() != _cnst) {
	            difftype = true;
	        }
        }
		return (difftype);
	}

private:
	UCNST _cnst;
};

std::string Effect::getName(bool tip, NameFlags::Value flags, int recursion) const
{
    //printf_log("effect getName() parent trigger= %d.\n",parent_trigger_id);
    if (!tip) {
	    return (type < scen.pergame->max_effect_types) ? getTypeName(type, false) : "Unknown!";
	} else {
        std::string stype = std::string("");
        std::ostringstream convert;
        switch (type) {
        case EffectType::None:
            // Let this act like a separator
            convert << "                                                                                    ";
            stype.append(convert.str());
            break;
        case EffectType::ChangeDiplomacy:
            convert << playerPronoun(s_player);
            switch (diplomacy) {
            case 0:
                convert << " allies with ";
                break;
            case 1:
                convert << " becomes neutral to ";
                break;
            case 2:
                convert << " <INVALID DIPLOMACY> to ";
                break;
            case 3:
                convert << " declares war on ";
                break;
            }
            convert << playerPronoun(t_player);
            stype.append(convert.str());
            break;
        case EffectType::ResearchTechnology:
            {
                bool hastech = pTech && pTech->id();
                std::wstring wtechname;
                std::string techname;
                if (hastech) {
                    wtechname = std::wstring(pTech->name());
                    techname = std::string(wtechname.begin(), wtechname.end());
                    if (panel >= 1 && panel <= 3) {
                        switch (panel) {
                        case 1:
                            convert << "enable " << playerPronoun(s_player) << " to research " << techname;
                            break;
                        case 2:
                            convert << "disable " << playerPronoun(s_player) << " to research " << techname;
                            break;
                        case 3:
                            convert << "enable " << playerPronoun(s_player) << " to research " << techname << " regardless of civ";
                            break;
                        }
                    } else {
                        convert << playerPronoun(s_player);
                        convert << " researches " << techname;
                    }
                } else {
                    convert << "INVALID";
                }
            }
            stype.append(convert.str());
            break;
        case EffectType::SendChat:
            switch (s_player) {
            case -1:
            case 0:
                convert << "tell everyone";
                break;
            default:
                convert << "tell p" << s_player;
            }
            convert << " \"" << text.c_str() << "\"";
            stype.append(convert.str());
            break;
        case EffectType::Sound:
            convert << "play sound " << sound.c_str();
            stype.append(convert.str());
            break;
        case EffectType::SendTribute:
            {
                std::string amount_string;
                if (amount == TS_LONG_MAX || amount == TS_LONG_MIN) {
                    amount_string = "max";
                } else {
                    if (amount < 0) {
                        amount_string = longToString(-amount);
                    } else {
                        amount_string = longToString(amount);
                    }
                }
                if (amount >= 0) {
                    convert << playerPronoun(s_player);
                    if (t_player == 0) {
                        convert << " loses";
                    } else {
                        convert << " gives ";
                        convert << "p" << t_player;
                    }
                    convert << " " << amount_string << " ";
                } else {
                    if (t_player == 0) {
                        if (res_type < 4) {
                            convert << "p" << s_player << " silently gets " << amount_string << " ";
                        } else {
                            convert << "p" << s_player << " gets " << amount_string << " ";
                        }
                    } else {
                        convert << "p" << t_player << " silently gives ";
                        convert << playerPronoun(s_player);
                        convert << " " << amount_string << " ";
                    }
                }
                switch (res_type) {
                case 0: // Food
                    convert << "food";
                    break;
                case 1: // Wood
                    convert << "wood";
                    break;
                case 2: // Stone
                    convert << "stone";
                    break;
                case 3: // Gold
                    convert << "gold";
                    break;
                case 20: // Units killed
                    if (amount == 1) {
                        convert << "kill";
                    } else {
                        convert << "kills";
                    }
                    break;
                default:
                    //convert << types_short[type];
                    if (res_type >= 0) {
                        const Link * list = esdata.resources.head();
	                    for (int i=0; list; list = list->next(), i++)
	                    {
		                    if (i == res_type) {
                                std::wstring resname(list->name());
                                convert << std::string( resname.begin(), resname.end());
		                        break;
		                    }
	                    }
	                }
                    break;
                }
                if (amount >= 0 && t_player > 0 && res_type < 4) {
                    convert << " (displays tribute alert)";
                }
                stype.append(convert.str());
            }
            break;
        case EffectType::ActivateTrigger:
        case EffectType::DeactivateTrigger:
            //stype.append(types_short[type]);
            if (trig_index != (unsigned)-1 && trig_index != (unsigned)-2) {
                if (trig_index < scen.triggers.size() && trig_index >= 0) {
                    if (trig_index == parent_trigger_id) {
                        switch (type) {
                        case EffectType::ActivateTrigger:
                            stype.append("repeat");
                            break;
                        case EffectType::DeactivateTrigger:
                            stype.append("don't repeat");
                            break;
                        }
                    } else {
                        if (scen.triggers.at(trig_index).loop) {
                            switch (type) {
                            case EffectType::ActivateTrigger:
                                stype.append("resume");
                                break;
                            case EffectType::DeactivateTrigger:
                                stype.append("pause");
                                break;
                            }
                        } else {
                            switch (type) {
                            case EffectType::ActivateTrigger:
                                stype.append("activate");
                                break;
                            case EffectType::DeactivateTrigger:
                                stype.append("deactivate");
                                break;
                            }
                        }
                        if (setts.showdisplayorder || setts.showtrigids) {
                            stype.append(" ").append(scen.triggers.at(trig_index).getIDName(true));
                        }
                        if (recursion > 0) {
                            stype.append(" ").append(scen.triggers.at(trig_index).getName(setts.showtrigfunction,true,recursion - 1));
                        }
                    }
                } else {
                    switch (type) {
                    case EffectType::ActivateTrigger:
                        stype.append("activate ");
                        break;
                    case EffectType::DeactivateTrigger:
                        stype.append("deactivate ");
                        break;
                    }
                    stype.append("<?>");
                }
                //convert << trig_index;
                //stype.append(convert.str());
            }
            break;
        case EffectType::AIScriptGoal:
            switch (ai_goal) {
            default:
                if (ai_goal >= -258 && ai_goal <= -3) {
                    // AI Shared Goal
                    convert << "complete AI shared goal " << ai_goal + 258;
                } else if (ai_goal >= 774 && ai_goal <= 1029) {
	                // Set AI Signal
                    convert << "signal AI? " << ai_goal - 774;
                } else {
                    convert << "signal AI " << ai_goal;
                }
            }
            stype.append(convert.str());
            break;;
        case EffectType::ChangeView:
            if (location.x >= 0 && location.y >= 0 && s_player >= 1) {
                convert << "change view for p" << s_player << " to " << location.x << "," << location.y;
            } else {
                convert << "INVALID";
            }
            stype.append(convert.str());
            break;
        case EffectType::UnlockGate:
        case EffectType::LockGate:
        case EffectType::KillObject:
        case EffectType::RemoveObject:
        case EffectType::FreezeUnit:
            switch (type) {
            case EffectType::UnlockGate:
                convert << "unlock";
                break;
            case EffectType::LockGate:
                convert << "lock";
                break;
            case EffectType::KillObject:
                convert << "kill";
                break;
            case EffectType::RemoveObject:
                convert << "remove";
                break;
            case EffectType::Unload:
                convert << "unload";
                break;
            case EffectType::FreezeUnit:
                if (scen.game == UP) {
                    switch (panel) {
                    case 1:
                        convert << "aggressive stance";
                        break;
                    case 2:
                        convert << "defensive stance";
                        break;
                    case 3:
                        convert << "stand ground";
                        break;
                    case 4:
                        convert << "no attack stance (no halt)";
                        break;
                    default:
                        convert << "no attack stance";
                        break;
                    }
                } else {
                    convert << "no attack stance";
                }
                break;
            }
            convert << " " << selectedUnits();
            stype.append(convert.str());
            break;
        case EffectType::Unload:
            convert << "unload " << selectedUnits() << " to " << location.x << "," << location.y;
            stype.append(convert.str());
            break;

        case EffectType::StopUnit:
            if (panel >= 1 && panel <= 9) {
                convert << "place" << " " << selectedUnits() << " into control group " << panel;
            } else {
                convert << "stop " << selectedUnits();
                stype.append(convert.str());
                break;
            }
            stype.append(convert.str());
            break;
        case EffectType::PlaceFoundation:
            convert << "place " << playerPronoun(s_player);
            if (pUnit && pUnit->id()) {
                std::wstring unitname(pUnit->name());
                std::string un(unitname.begin(), unitname.end());
                convert << " " << un;
            } else {
                convert << " INVALID EFFECT";
            }
            convert << " foundation at " << location.x << "," << location.y;
            stype.append(convert.str());
            break;
        case EffectType::CreateObject:
            convert << "create";
            convert << " " << playerPronoun(s_player);
            if (pUnit && pUnit->id()) {
                std::wstring unitname(pUnit->name());
                std::string un(unitname.begin(), unitname.end());
                convert << " " << un;
            } else {
                convert << " INVALID EFFECT";
            }
            convert << " at " << location.x << "," << location.y;
            stype.append(convert.str());
            break;
        case EffectType::Patrol:
            try
            {
                // keep in mind multiple units can possess the same id, but
                // it only operates on the last farm with that id.
                //
                // s_player may not be selected. Therefore, go through all
                // the units in scenario.
                //
                // A gaia farm could be made infinite.
                //Player * p = scen.players + s_player;
                //for (vector<Unit>::const_iterator iter = p->units.begin(); iter != sorted.end(); ++iter) {
                //}

                if (num_sel > 0) {
                    std::vector<PlayersUnit> all (num_sel);
	                for (int i = 0; i < num_sel; i++) {
                        all[i] = find_map_unit(uids[i]);
	                }
                    std::vector<PlayersUnit> farms (num_sel);
                    std::vector<PlayersUnit> other (num_sel);

                    std::vector<PlayersUnit>::iterator it;
                    it = copy_if (all.begin(), all.end(), farms.begin(), playersunit_ucnst_equals(50) );
                    farms.resize(std::distance(farms.begin(),it));  // shrink container to new size

                    it = copy_if (all.begin(), all.end(), other.begin(), playersunit_ucnst_notequals(50) );
                    other.resize(std::distance(other.begin(),it));  // shrink container to new size

                    if (farms.size() > 0) {
                        convert << "reseed ";
                        for(std::vector<PlayersUnit>::iterator it = farms.begin(); it != farms.end(); ++it) {
                            convert << it->u->ident << " (" <<
                                get_unit_full_name(it->u->ident)
                                << ")" << " ";
                        }
	                }

                    if (other.size() > 0) {
                        if (!valid_location_coord() && null_location_unit()) {
                            convert << "stop ";
                        } else {
                            convert << "patrol ";
                        }
                        for(std::vector<PlayersUnit>::iterator it = other.begin(); it != other.end(); ++it) {
                            convert << it->u->ident << " (" <<
                                get_unit_full_name(it->u->ident)
                                << ")" << " ";
                        }
	                }
	            } else {
                    convert << "patrol / reseed " << selectedUnits();
	            }
                stype.append(convert.str());
            }
	        catch (std::exception& ex)
	        {
                stype.append(ex.what());
	        }
            break;
        case EffectType::ChangeOwnership:
            convert << playerPronoun(t_player) << " now owns " << selectedUnits();
            stype.append(convert.str());
            break;
        case EffectType::TaskObject:
            if (!valid_location_coord() && null_location_unit()) {
                convert << "stop" << " " << selectedUnits();
                stype.append(convert.str());
                break;
            }

            convert << "task";
            convert << " " << selectedUnits();
            if (valid_location_coord()) {
                convert << " to " << location.x << "," << location.y;
            } else {
                convert << " to unit " << uid_loc << " (" << get_unit_full_name(uid_loc) << ")";
            }
            stype.append(convert.str());
            break;
        case EffectType::DeclareVictory:
            convert << "p" << s_player << " victory";
            stype.append(convert.str());
            break;
        case EffectType::DisplayInstructions:
            convert << "instruct players \"" << text.c_str() << "\"";
            stype.append(convert.str());
            break;
        case EffectType::ChangeObjectName:
            stype.append("rename '");
            stype.append(text.c_str());
            stype.append("'");
            convert << " " << selectedUnits();
            stype.append(convert.str());
            break;
        case EffectType::DamageObject:
            {
                if (amount == TS_LONG_MAX) {
                    convert << "min health for " << selectedUnits();
                } else if (amount == TS_LONG_MIN) {
                    convert << "make " << selectedUnits() << " invincible";
                } else if (isFloorAmount()) {
                    convert << "cap health of " << selectedUnits() << " to " << (amount - TS_FLOAT_MIN) << " HP (Part 1 of 2)";
                } else if (isCeilAmount()) {
                    convert << "cap health of " << selectedUnits() << " to " << (TS_FLOAT_MAX - amount) << " HP (Part 2 of 2)";
                } else {
                    if (amount < 0) {
                        convert << "buff " << selectedUnits() << " with " << -amount << " HP";
                    } else {
                        convert << "damage " << selectedUnits() << " by " << amount << " HP";
                    }
                }
                stype.append(convert.str());
            }
            break;
        case EffectType::ChangeObjectHP:
        case EffectType::ChangeObjectAttack:
            if (amount > 0) {
                convert << "+";
            }
            convert << amount << " " << getTypeName(type, true) << " to "  << selectedUnits();
            stype.append(convert.str());
            break;
        case EffectType::ChangeSpeed_UP: // SnapView_SWGB, AttackMove_HD
            switch (scen.game) {
            case UP:
                if (amount > 0) {
                    convert << "+";
                }
                convert << amount << " " << getTypeName(type, true) << " to "  << selectedUnits();
                stype.append(convert.str());
                break;
            case SWGB:
            case SWGBCC:
                if (location.x >= 0 && location.y >= 0 && s_player >= 1) {
                    convert << "snap view for p" << s_player << " to " << location.x << "," << location.y;
                } else {
                    convert << "INVALID";
                }
                stype.append(convert.str());
                break;
            case AOHD:
            case AOF:
            case AOHD4:
            case AOF4:
            case AOHD6:
            case AOF6:
                if (!valid_location_coord() && null_location_unit()) {
                    convert << "stop?" << " " << selectedUnits();
                    stype.append(convert.str());
                    break;
                }

                convert << "attack-move";
                convert << " " << selectedUnits();
                if (valid_location_coord()) {
                    convert << " to " << location.x << "," << location.y;
                } else {
                    convert << " to unit " << uid_loc << " (" << get_unit_full_name(uid_loc) << ")";
                }
                stype.append(convert.str());
                break;
            default:
                stype.append((type < scen.pergame->max_effect_types) ? getTypeName(type, true) : "Unknown!");
                break;
            }
            break;
        case EffectType::ChangeRange_UP: // DisableAdvancedButtons_SWGB, ChangeArmor_HD
        case EffectType::ChangeMeleArmor_UP: // ChangeRange_HD, EnableTech_SWGB
        case EffectType::ChangePiercingArmor_UP: // ChangeSpeed_HD, DisableTech_SWGB
            switch (scen.game) {
            case AOF:
            case AOHD:
	        case AOHD4:
	        case AOF4:
	        case AOHD6:
	        case AOF6:
            case UP:
                if (amount > 0) {
                    convert << "+";
                }
                convert << amount << " " << getTypeName(type, true) << " to "  << selectedUnits();
                stype.append(convert.str());
                break;
            case SWGB:
            case SWGBCC:
                {
                    switch (type) {
                    case EffectType::DisableAdvancedButtons_SWGB:
                        stype.append((type < scen.pergame->max_effect_types) ? getTypeName(type, true) : "Unknown!");
                        break;
                    case EffectType::EnableTech_SWGB:
                    case EffectType::DisableTech_SWGB:
                        {
                            bool hastech = pTech && pTech->id();
                            std::wstring wtechname;
                            std::string techname;
                            if (hastech) {
                                wtechname = std::wstring(pTech->name());
                                techname = std::string(wtechname.begin(), wtechname.end());
                                switch (type) {
                                case EffectType::EnableTech_SWGB:
                                    convert << "enable " << playerPronoun(s_player) << " to research " << techname;
                                    break;
                                case EffectType::DisableTech_SWGB:
                                    convert << "disable " << playerPronoun(s_player) << " to research " << techname;
                                    break;
                                }
                            } else {
                                convert << "INVALID TECHNOLOGY";
                            }
                        }
                        break;
                    }
                }
                stype.append(convert.str());
                break;
            default:
                stype.append((type < scen.pergame->max_effect_types) ? getTypeName(type, true) : "Unknown!");
                break;
            }
            break;
	    case EffectType::EnableUnit_SWGB: // EffectType::HealObject_HD:
	        switch (scen.game) {
	        case AOHD:
	        case AOF:
	        case AOHD4:
	        case AOF4:
	        case AOHD6:
	        case AOF6:
                convert << "heal " << selectedUnits() << " by " << amount;
                stype.append(convert.str());
	            break;
	        default:
                stype.append((type < scen.pergame->max_effect_types) ? getTypeName(type, true) : "Unknown!");
                break;
	        }
            break;
	    case EffectType::FlashUnit_SWGB:   // ChangeUnitStance_HD
	        switch (scen.game) {
	        case AOHD:
	        case AOF:
	        case AOHD4:
	        case AOF4:
	        case AOHD6:
	        case AOF6:
                switch (stance) {
                case -1:
                    convert << "NULL stance";
                    break;
                case 0:
                    convert << "aggressive stance";
                    break;
                case 1:
                    convert << "defensive stance";
                    break;
                case 2:
                    convert << "stand ground";
                    break;
                case 3:
                    convert << "no attack stance";
                    break;
                }
                convert << " " << selectedUnits();
                stype.append(convert.str());
                break;
	        default:
                stype.append((type < scen.pergame->max_effect_types) ? getTypeName(type, true) : "Unknown!");
                break;
	        }
	        break;
	    case EffectType::DisableUnit_SWGB: // TeleportObject_HD
	        switch (scen.game) {
	        case AOHD:
	        case AOF:
	        case AOHD4:
	        case AOF4:
	        case AOHD6:
	        case AOF6:
                convert << "teleport one of " << selectedUnits() << " to " << location.x << "," << location.y;
                stype.append(convert.str());
	            break;
	        default:
                stype.append((type < scen.pergame->max_effect_types) ? getTypeName(type, true) : "Unknown!");
                break;
	        }
            break;
	    case EffectType::InputOff_CC:
	    case EffectType::InputOn_CC:
            stype.append((type < scen.pergame->max_effect_types) ? getTypeName(type, true) : "Unknown!");
            break;
        default:
            stype.append((type < scen.pergame->max_effect_types) ? getTypeName(type, true) : "Unknown!");
        }

        return flags&NameFlags::LIMITLEN?stype.substr(0,MAX_CHARS):stype;
    }
}

const char * Effect::getTypeName(size_t type, bool concise) const
{
	return concise?types_short[type]:types[type];
}

int Effect::getPlayer() const
{
	return s_player;
}

void Effect::setPlayer(int player)
{
	s_player = player;
}

inline bool Effect::valid_full_map() const {
    return (area.left == -1 && area.right == -1 && area.bottom == -1 && area.top == -1);
}

inline bool Effect::valid_partial_map() const {
    return (area.left >=  0 && area.right >= area.left && area.bottom >=  0 && area.top >= area.bottom);
}

inline bool Effect::valid_area_location() const {
    return area.left == area.right && area.top == area.bottom;
}

inline bool Effect::valid_area() const {
    return valid_full_map() || valid_partial_map();
}

inline bool Effect::has_selected() const {
    return num_sel > 0;
}

inline bool Effect::valid_selected() const {
	for (int i = 0; i < num_sel; i++) {
	    if (!valid_unit_id(uids[i])) {
		    return false;
	    }
	}
	return true;
}

inline bool Effect::has_unit_constant() const {
    return pUnit;
    //return ucnst >= 0;
}

inline bool Effect::valid_unit_constant() const {
    return pUnit && pUnit->id() >= 0;
    //return ucnst >= 0;
}

inline bool Effect::valid_unit_spec() const {
    if (!valid_source_player()) {
        return false;
    }
    if (has_unit_constant() && !valid_unit_constant()) {
        return false;
    }
    //utype >= 0 || group >= 0
    return true;
}

inline bool Effect::valid_technology_spec() const {
	return pTech != NULL && pTech->id() >= 0;;
}

inline bool Effect::valid_location_coord() const {
    return location.x >= 0 && location.y >= 0;
}

inline bool Effect::null_location_unit() const {
	return uid_loc == -1;
}

inline bool Effect::valid_location_unit() const {
	return valid_unit_id(uid_loc);
}

inline bool Effect::valid_source_player() const {
    return s_player >= 0 && s_player <= 8;
}

inline bool Effect::valid_target_player() const {
    return t_player >= 0 && t_player <= 8;
}

inline bool Effect::valid_trigger() const {
    return trig_index >= 0 &&  trig_index < scen.triggers.size();
}

inline bool Effect::valid_panel() const {
	// panel == -1 is acceptable because Azzzru's scripts omit panel
	// to shorten SCX file and scenario still works fine.
    return panel >= -1;
}

inline bool Effect::valid_destination() const {
	return valid_location_coord() || valid_unit_id(uid_loc);
}

/*
 * Amount can be negative, but can't be -1 (or can it? it appears red in
 * scenario editor. It CAN be -1 (in AOHD at least)
 */
inline bool Effect::valid_points() const {
	//return (amount != -1);
	return true;
}

bool Effect::get_valid_since_last_check() {
    return valid_since_last_check;
}

bool Effect::check_and_save()
{
    return valid_since_last_check = check();
}

/*
 * False positives are better than false negatives.
 */
bool Effect::check() const
{
    if (type < 1 || type >= scen.pergame->max_effect_types) {
        return false;
    }

    bool valid_selected = Effect::valid_selected();
    bool has_valid_selected = Effect::has_selected() && valid_selected; // perform this check on all effects
    bool valid_area_selection = valid_unit_spec() && valid_area();

    if (!valid_selected) {
        return false;
    }

	switch (type)
	{
	case EffectType::ChangeDiplomacy:
		return (valid_source_player() && valid_source_player() && diplomacy >= 0);

	case EffectType::ResearchTechnology:
		return (valid_source_player() && valid_technology_spec());

	case EffectType::SendChat:
		return (valid_source_player() && *text.c_str());	//AOK missing text check

	case EffectType::Sound:
		return (valid_source_player() && sound.length());	//AOK missing sound check

	case EffectType::SendTribute:
		return (valid_source_player() && valid_target_player() && res_type >= 0);

	case EffectType::UnlockGate:
	case EffectType::LockGate:
		return has_valid_selected;

	case EffectType::ActivateTrigger:
	case EffectType::DeactivateTrigger:
		return valid_trigger();

	case EffectType::AIScriptGoal:
		return true;

	case EffectType::CreateObject:
		return valid_source_player() && valid_location_coord() && valid_unit_spec();

	case EffectType::Unload:
        return (has_valid_selected || valid_area_selection) && valid_destination();

	case EffectType::TaskObject:
        return (valid_area() || has_valid_selected ||
               (!valid_location_coord() && null_location_unit()));

	case EffectType::KillObject:
	case EffectType::RemoveObject:
	case EffectType::FreezeUnit:
	case EffectType::StopUnit:
	case EffectType::FlashUnit_SWGB:
		return has_valid_selected || valid_area_selection;

	case EffectType::DeclareVictory:
		return valid_source_player();

	case EffectType::ChangeView:
		return valid_source_player() && valid_location_coord();

	case EffectType::ChangeOwnership:
		return (has_valid_selected || valid_area_selection) && valid_target_player();

	case EffectType::Patrol:
		return (has_valid_selected || valid_area_selection) && valid_location_coord();

	case EffectType::DisplayInstructions:
		return (valid_panel() && disp_time >= 0 && (*text.c_str() || textid));	//AOK missing text

	case EffectType::ClearInstructions:
		return valid_panel();

	case EffectType::UseAdvancedButtons:
		return true;

	case EffectType::DamageObject:
	case EffectType::ChangeObjectHP:
	case EffectType::ChangeObjectAttack:
		return (has_valid_selected || valid_area_selection) && valid_points();

	case EffectType::ChangeSpeed_UP: // SnapView_SWGB // AttackMove_HD
	    switch (scen.game) {
	    case UP:
		    return (has_valid_selected || valid_area_selection) && valid_points();
	    case AOHD:
	    case AOF:
	    case AOHD4:
	    case AOF4:
	    case AOHD6:
	    case AOF6:
            return (valid_area() || has_valid_selected ||
                    (!valid_location_coord() && null_location_unit()));
	    case SWGB:
	    case SWGBCC:
		    return valid_source_player() && valid_location_coord();
	    }
		return valid_location_coord();

	case EffectType::ChangeRange_UP: // DisableAdvancedButtons_SWGB // ChangeArmor_HD
	    switch (scen.game) {
	    case UP:
	    case AOHD:
	    case AOF:
	    case AOHD4:
	    case AOF4:
	    case AOHD6:
	    case AOF6:
		    return (has_valid_selected || valid_area_selection) && valid_points();
	    case SWGB:
	    case SWGBCC:
		    return true;
	    }
		return valid_location_coord();

	case EffectType::ChangeMeleArmor_UP: // ChangeRange_HD // EnableTech_SWGB
	case EffectType::ChangePiercingArmor_UP: // ChangeSpeed_HD // DisableTech_SWGB
	    switch (scen.game) {
	    case UP:
	    case AOHD:
	    case AOF:
	    case AOHD4:
	    case AOF4:
	    case AOHD6:
	    case AOF6:
		    return (has_valid_selected || valid_area_selection) && valid_points();
	    case SWGB:
	    case SWGBCC:
		    return valid_technology_spec();
	    }
		return true;

	case EffectType::PlaceFoundation:
		return (valid_source_player() && valid_unit_spec() && valid_location_coord());

	case EffectType::ChangeObjectName:
	    switch (scen.game) {
	    case AOK:
	    case AOC:
	    case AOHD:
	    case AOF:
	    case AOHD4:
	    case AOF4:
	    case AOHD6:
	    case AOF6:
	        return (has_valid_selected || valid_area_selection);
	    }
		return true;

	case EffectType::EnableUnit_SWGB:
	case EffectType::DisableUnit_SWGB:
		return valid_unit_spec();

	default:
		return false;	//unknown effect type
	}
}

void Effect::accept(TriggerVisitor& tv)
{
	tv.visit(*this);
}

const char *Effect::types_aok[] = {
	"",
	"Change Diplomacy",
	"Research Technology",
	"Send Chat",
	"Play Sound",
	"Send Tribute",
	"Unlock Gate",
	"Lock Gate",
	"Activate Trigger",
	"Deactivate Trigger",
	"AI Script Goal",
	"Create Object",
	"Task Object",
	"Declare Victory",
	"Kill Object (Health=0,deselect)",
	"Remove Object",
	"Change View",
	"Unload",
	"Change Ownership",
	"Patrol Units / Reseed Farms",
	"Display Instructions",
	"Clear Instructions",
	"Freeze Unit (No Attack Stance)",
	"Use Advanced Buttons"
};

const char *Effect::types_aoc[] = {
	"",
	"Change Diplomacy",
	"Research Technology",
	"Send Chat",
	"Play Sound",
	"Send Tribute",
	"Unlock Gate",
	"Lock Gate",
	"Activate Trigger",
	"Deactivate Trigger",
	"AI Script Goal",
	"Create Object",
	"Task Object",
	"Declare Victory",
	"Kill Object (Health=0,deselect)",
	"Remove Object",
	"Change View",
	"Unload",
	"Change Ownership",
	"Patrol Units / Reseed Farms",
	"Display Instructions",
	"Clear Instructions",
	"Freeze Unit (No Attack Stance)",
	"Use Advanced Buttons",
	"Damage Object",
	"Place Foundation",
	"Change Object Name",
	"Change Object HP (Change Max)",
	"Change Object Attack",
	"Stop Unit"
};

const char *Effect::types_up[] = {
	"",
	"Change Diplomacy",
	"Research Technology",
	"Send Chat",
	"Play Sound",
	"Send Tribute",
	"Unlock Gate",
	"Lock Gate",
	"Activate Trigger",
	"Deactivate Trigger",
	"AI Script Goal",
	"Create Object",
	"Task Object",
	"Declare Victory",
	"Kill Object (Health=0,deselect)",
	"Remove Object",
	"Change View",
	"Unload",
	"Change Ownership",
	"Patrol Units / Reseed Farms",
	"Display Instructions",
	"Clear Instructions",
	"Freeze Unit (No Attack Stance)",
	"Use Advanced Buttons",
	"Damage Object",
	"Place Foundation",
	"Change Object Name",
	"Change Object HP (Change Max)",
	"Change Object Attack",
	"Stop Unit",
	"Change Speed",
	"Change Range",
	"Change Mele Armor",
	"Change Piercing Armor"
};

const char *Effect::types_swgb[] = {
	"",
	"Change Alliance",
	"Research Technology",
	"Send Chat",
	"Play Sound",
	"Tribute",
	"Unlock Gate",
	"Lock Gate",
	"Activate Trigger",
	"Deactivate Trigger",
	"AI Script Goal",
	"Create Object",
	"Task Object",
	"Declare Victory",
	"Kill Object (Health=0,deselect)",
	"Remove Object",
	"Scroll View",
	"Unload",
	"Change Ownership",
	"Patrol Units",
	"Display Instructions",
	"Clear Instructions",
	"Freeze Unit (No Attack Stance)",
	"Enable Advanced Buttons",
	"Damage Object (Change Health)",
	"Place Foundation",
	"Change Object Name",
	"Change Object HP (Change Max)",
	"Change Object Attack",
	"Stop Unit",
	"Snap View",
	"Disable Advanced Buttons",
	"Enable Tech",
	"Disable Tech",
	"Enable Unit",
	"Disable Unit",
	"Flash Objects"
};

const char *Effect::types_cc[] = {
	"",
	"Change Alliance",
	"Research Technology",
	"Send Chat",
	"Play Sound",
	"Tribute",
	"Unlock Gate",
	"Lock Gate",
	"Activate Trigger",
	"Deactivate Trigger",
	"AI Script Goal",
	"Create Object",
	"Task Object",
	"Declare Victory",
	"Kill Object (Health=0,deselect)",
	"Remove Object",
	"Scroll View",
	"Unload",
	"Change Ownership",
	"Patrol Units",
	"Display Instructions",
	"Clear Instructions",
	"Freeze Unit (No Attack Stance)",
	"Enable Advanced Buttons",
	"Damage Object (Change Health)",
	"Place Foundation",
	"Change Object Name",
	"Change Object HP (Change Max)",
	"Change Object Attack",
	"Stop Unit",
	"Snap View",
	"Disable Advanced Buttons",
	"Enable Tech",
	"Disable Tech",
	"Enable Unit",
	"Disable Unit",
	"Flash Objects",
	"Input Off",
	"Input On"
};

const char *Effect::types_aohd[] = {
	"",
	"Change Diplomacy",
	"Research Technology",
	"Send Chat",
	"Play Sound",
	"Send Tribute",
	"Unlock Gate",
	"Lock Gate",
	"Activate Trigger",
	"Deactivate Trigger",
	"AI Script Goal",
	"Create Object",
	"Task Object",
	"Declare Victory",
	"Kill Object (Health=0,deselect)",
	"Remove Object",
	"Change View",
	"Unload",
	"Change Ownership",
	"Patrol Units / Reseed Farms",
	"Display Instructions",
	"Clear Instructions",
	"Freeze Unit (No Attack Stance)",
	"Use Advanced Buttons",
	"Damage Object (Change Health)",
	"Place Foundation",
	"Change Object Name",
	"Change Object HP (Change Max)",
	"Change Object Attack",
	"Stop Unit",
	"Attack-Move",
	"Change Armor",
	"Change Range",
	"Change Speed",
	"Heal Object",
	"Teleport One Object",
	"Change Unit Stance"
};

const char *Effect::types_aof[] = {
	"",
	"Change Diplomacy",
	"Research Technology",
	"Send Chat",
	"Play Sound",
	"Send Tribute",
	"Unlock Gate",
	"Lock Gate",
	"Activate Trigger",
	"Deactivate Trigger",
	"AI Script Goal",
	"Create Object",
	"Task Object",
	"Declare Victory",
	"Kill Object (Health=0,deselect)",
	"Remove Object",
	"Change View",
	"Unload",
	"Change Ownership",
	"Patrol Units / Reseed Farms",
	"Display Instructions",
	"Clear Instructions",
	"Freeze Unit (No Attack Stance)",
	"Use Advanced Buttons",
	"Damage Object (Change Health)",
	"Place Foundation",
	"Change Object Name",
	"Change Object HP (Change Max)",
	"Change Object Attack",
	"Stop Unit",
	"Attack-Move",
	"Change Armor",
	"Change Range",
	"Change Speed",
	"Heal Object",
	"Teleport One Object",
	"Change Unit Stance"
};

const char *Effect::types_short_aok[] = {
	"",
	"Change Diplomacy",
	"Research",
	"Chat",
	"Sound",
	"Tribute",
	"Unlock Gate",
	"Lock Gate",
	"Activate",
	"Deactivate",
	"AI Script Goal",
	"Create",
	"Task",
	"Declare Victory",
	"Kill",
	"Remove",
	"Change View",
	"Unload",
	"Change Ownership",
	"Patrol / Reseed",
	"Instructions",
	"Clear Instructions",
	"Freeze (No Attack)",
	"Use Advanced Buttons"
};

const char *Effect::types_short_aoc[] = {
	"",
	"Change Diplomacy",
	"Research",
	"Chat",
	"Sound",
	"Tribute",
	"Unlock Gate",
	"Lock Gate",
	"Activate",
	"Deactivate",
	"AI Script Goal",
	"Create",
	"Task",
	"Declare Victory",
	"Kill",
	"Remove",
	"Change View",
	"Unload",
	"Change Ownership",
	"Patrol / Reseed",
	"Instructions",
	"Clear Instructions",
	"Freeze (No Attack)",
	"Use Advanced Buttons",
	"Damage",
	"Place Foundation",
	"Rename",
	"HP",
	"Attack",
	"Stop Unit",
};

const char *Effect::types_short_up[] = {
	"",
	"Change Diplomacy",
	"Research",
	"Chat",
	"Sound",
	"Tribute",
	"Unlock Gate",
	"Lock Gate",
	"Activate",
	"Deactivate",
	"AI Script Goal",
	"Create",
	"Task",
	"Declare Victory",
	"Kill",
	"Remove",
	"Change View",
	"Unload",
	"Change Ownership",
	"Patrol / Reseed",
	"Instructions",
	"Clear Instructions",
	"Freeze (No Attack)",
	"Use Advanced Buttons",
	"Damage",
	"Place Foundation",
	"Rename",
	"HP",
	"Attack",
	"Stop Unit",
	"Speed",
	"Range",
	"Mele Armor",
	"Piercing Armor"
};

const char *Effect::types_short_aohd[] = {
	"",
	"Change Diplomacy",
	"Research",
	"Chat",
	"Sound",
	"Tribute",
	"Unlock Gate",
	"Lock Gate",
	"Activate",
	"Deactivate",
	"AI Script Goal",
	"Create",
	"Task",
	"Declare Victory",
	"Kill",
	"Remove",
	"Change View",
	"Unload",
	"Change Ownership",
	"Patrol / Reseed",
	"Instructions",
	"Clear Instructions",
	"Freeze (No Attack)",
	"Use Advanced Buttons",
	"Damage",
	"Place Foundation",
	"Rename",
	"HP",
	"Attack",
	"Stop Unit",
	"Attack-Move",
	"Armor",
	"Range",
	"Speed",
	"Heal",
	"Teleport One",
	"Change Stance"
};

const char *Effect::types_short_aof[] = {
	"",
	"Change Diplomacy",
	"Research",
	"Chat",
	"Sound",
	"Tribute",
	"Unlock Gate",
	"Lock Gate",
	"Activate",
	"Deactivate",
	"AI Script Goal",
	"Create",
	"Task",
	"Declare Victory",
	"Kill",
	"Remove",
	"Change View",
	"Unload",
	"Change Ownership",
	"Patrol / Reseed",
	"Instructions",
	"Clear Instructions",
	"Freeze (No Attack)",
	"Use Advanced Buttons",
	"Damage",
	"Place Foundation",
	"Rename",
	"HP",
	"Attack",
	"Stop Unit",
	"Attack-Move",
	"Armor",
	"Range",
	"Speed",
	"Heal",
	"Teleport One",
	"Change Stance"
};

const char *Effect::types_short_swgb[] = {
	"",
	"Change Alliance",
	"Research",
	"Chat",
	"Sound",
	"Tribute",
	"Unlock Gate",
	"Lock Gate",
	"Activate",
	"Deactivate",
	"AI Script Goal",
	"Create",
	"Task",
	"Declare Victory",
	"Kill",
	"Remove",
	"Scroll View",
	"Unload",
	"Change Ownership",
	"Patrol",
	"Instructions",
	"Clear Instructions",
	"Freeze (No Attack)",
	"Use Advanced Buttons",
	"Damage",
	"Place Foundation",
	"Rename",
	"HP",
	"Attack",
	"Stop Unit",
	"Snap View",
	"Disable Advanced",
	"Enable Tech",
	"Disable Tech",
	"Enable Unit",
	"Disable Unit",
	"Flash"
};

const char *Effect::types_short_cc[] = {
	"",
	"Change Alliance",
	"Research",
	"Chat",
	"Sound",
	"Tribute",
	"Unlock Gate",
	"Lock Gate",
	"Activate",
	"Deactivate",
	"AI Script Goal",
	"Create",
	"Task",
	"Declare Victory",
	"Kill",
	"Remove",
	"Scroll View",
	"Unload",
	"Change Ownership",
	"Patrol",
	"Instructions",
	"Clear Instructions",
	"Freeze (No Attack)",
	"Use Advanced Buttons",
	"Damage",
	"Place Foundation",
	"Rename",
	"HP",
	"Attack",
	"Stop Unit",
	"Snap View",
	"Disable Advanced",
	"Enable Tech",
	"Disable Tech",
	"Enable Unit",
	"Disable Unit",
	"Flash",
	"Input Off",
	"Input On"
};

const char *Effect::virtual_types_up[] = {
    "",
    "Enable Object",
    "Disable Object",
    "Enable Technology",
    "Disable Technology",
    "Enable Tech (Any Civ)",
    "Set HP",
    "Heal Object",
    "Aggressive Stance",
    "Defensive Stance",
    "Stand Ground",
    "No Attack Stance",
    "Resign",
    "Flash Objects",
    "Set AP",
    "Set Control Group 1",
    "Set Control Group 2",
    "Set Control Group 3",
    "Set Control Group 4",
    "Set Control Group 5",
    "Set Control Group 6",
    "Set Control Group 7",
    "Set Control Group 8",
    "Set Control Group 9",
    "Snap View",
    "Max Amount",
    "Min Amount",
    "Cap Health Part 1 of 2",
    "Cap Health Part 2 of 2",
    "Set AI Signal",
    "Set AI Shared Goal",
    "Enable Cheats",
};

const char *Effect::virtual_types_aoc[] = {
    "",
    "Max Amount",
    "Min Amount",
    "Cap Health Part 1 of 2",
    "Cap Health Part 2 of 2",
    "Set AI Signal",
    "Set AI Shared Goal",
    "Enable Cheats",
    "Freeze unit",
};

const char *Effect::virtual_types_aohd[] = {
    "",
    "Max Amount",
    "Min Amount",
    "Cap Health Part 1 of 2",
    "Cap Health Part 2 of 2",
    "Freeze unit",
};

const char *Effect::virtual_types_aof[] = {
    "",
    "Max Amount",
    "Min Amount",
    "Cap Health Part 1 of 2",
    "Cap Health Part 2 of 2",
    "Freeze unit",
};

const char *Effect::virtual_types_swgb[] = {
    "",
    "Max Amount",
    "Min Amount",
    "Cap Health Part 1 of 2",
    "Cap Health Part 2 of 2",
    "Freeze unit",
};

const char *Effect::virtual_types_cc[] = {
    "",
    "Max Amount",
    "Min Amount",
    "Cap Health Part 1 of 2",
    "Cap Health Part 2 of 2",
    "Freeze unit",
};

const char *Effect::virtual_types_aok[] = {
    "",
    "Max Amount",
    "Min Amount",
};

bool Effect::isCeilAmount() const {
    return amount < TS_FLOAT_MAX && amount >= (TS_FLOAT_MAX - TS_HP_MAX);
}

bool Effect::isFloorAmount() const {
    return amount > TS_FLOAT_MIN && amount <= (TS_FLOAT_MIN + TS_HP_MAX);
}

const char** Effect::types;
const char** Effect::types_short;
const char** Effect::virtual_types;
