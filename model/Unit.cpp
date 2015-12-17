/*
	Unit.cpp: defines Unit class functions

	MODEL
*/

#include "Unit.h"
#include "../util/utilio.h"
#include "../util/Buffer.h"
#include "../model/scen.h"

extern Scenario scen;

Unit::Unit(UID id)
:	x(0), y(0), z(1), ident(id), cnst(0),
	state(2), rotate(0), frame(0), garrison(-1),
	pType(NULL)
{
    if (scen.ver2 == SV2_AOK) {
        garrison = 0;
    }
}

Unit::Unit(Buffer &from)
{
	from.read(this, size);
	pType = esdata.units.getByIdSafe(cnst);
}

Unit::Unit(const Unit & spec)
{
    x = spec.x;
    y = spec.y;
    z = spec.z;
    ident = spec.ident;
    cnst = spec.cnst;
    state = spec.state;
    rotate = spec.rotate;
    frame = spec.frame;
    garrison = spec.garrison;
    setType(spec.getType());
}

void Unit::read(FILE *in)
{
	fread(this, size, 1, in);
	pType = esdata.units.getByIdSafe(cnst);
}

const UnitLink *Unit::getType()
{
	return (pType) ? pType : (pType = esdata.units.getById(cnst));
}

const UnitLink *Unit::getType() const
{
	return (pType) ? pType : esdata.units.getById(cnst);
}

void Unit::setType(const UnitLink *t)
{
	assert(t);
	pType = t;
	cnst = (UCNST)t->id();
}

void Unit::setType(UCNST new_cnst)
{
	this->cnst = new_cnst;
	pType = esdata.units.getByIdSafe(cnst);
}

UCNST Unit::getTypeID()
{
	return cnst;
}

void Unit::toBuffer(Buffer &to) const
{
	to.write(this, size);
}

size_t Unit::size = 29;
