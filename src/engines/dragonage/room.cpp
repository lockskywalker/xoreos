/* xoreos - A reimplementation of BioWare's Aurora engine
 *
 * xoreos is the legal property of its developers, whose names
 * can be found in the AUTHORS file distributed with this source
 * distribution.
 *
 * xoreos is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * xoreos is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with xoreos. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  A room in a Dragon Age: Origins area.
 */

#include "src/common/util.h"
#include "src/common/strutil.h"
#include "src/common/maths.h"
#include "src/common/error.h"
#include "src/common/transmatrix.h"

#include "src/aurora/resman.h"
#include "src/aurora/gff4file.h"

#include "src/graphics/aurora/model.h"

#include "src/events/events.h"

#include "src/engines/aurora/model.h"

#include "src/engines/dragonage/room.h"

namespace Engines {

namespace DragonAge {

static const uint32 kRMLID     = MKTAG('R', 'M', 'L', ' ');
static const uint32 kVersion40 = MKTAG('V', '4', '.', '0');

static const uint32 kMDLID     = MKTAG('M', 'D', 'L', ' ');

using ::Aurora::GFF4File;
using ::Aurora::GFF4Struct;
using ::Aurora::GFF4List;

using namespace ::Aurora::GFF4FieldNamesEnum;

Room::Room(const Aurora::GFF4Struct &room) {
	try {
		load(room);
	} catch (...) {
		clean();
		throw;
	}
}

Room::~Room() {
	try {
		clean();
	} catch (...) {
	}
}

int32 Room::getID() const {
	return _id;
}

void Room::clean() {
	hide();

	for (Models::iterator m = _models.begin(); m != _models.end(); ++m)
		delete *m;

	deindexResources(_resources);
}

void Room::load(const GFF4Struct &room) {
	_id = room.getSint(kGFF4EnvRoomID, -1);

	const Common::UString roomFile = room.getString(kGFF4EnvRoomFile);

	indexOptionalArchive(roomFile + ".rim"      , 12000, _resources);
	indexOptionalArchive(roomFile + ".gpu.rim"  , 12001, _resources);
	indexOptionalArchive(roomFile + "_0.rim"    , 12002, _resources);
	indexOptionalArchive(roomFile + "_0.gpu.rim", 12003, _resources);

	loadLayout(roomFile);
	loadLayout(roomFile + "_0");
	loadLayout(roomFile + "_1");
}

void Room::loadLayout(const Common::UString &roomFile) {
	if (!ResMan.hasResource(roomFile, Aurora::kFileTypeRML) || EventMan.quitRequested())
		return;

	GFF4File rml(roomFile, Aurora::kFileTypeRML, kRMLID);
	if (rml.getTypeVersion() != kVersion40)
		throw Common::Exception("Unsupported RML version %s", Common::debugTag(rml.getTypeVersion()).c_str());

	const GFF4Struct &rmlTop = rml.getTopLevel();

	float roomPos[3] = { 0.0f, 0.0f, 0.0f };
	rmlTop.getVector3(kGFF4Position, roomPos[0], roomPos[1], roomPos[2]);

	float roomOrient[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	rmlTop.getVector4(kGFF4Orientation, roomOrient[0], roomOrient[1], roomOrient[2], roomOrient[3]);
	roomOrient[3] = Common::rad2deg(acos(roomOrient[3]) * 2.0);

	Common::TransformationMatrix roomTransform;
	roomTransform.translate(roomPos[0], roomPos[1], roomPos[2]);
	roomTransform.rotate(roomOrient[3], roomOrient[0], roomOrient[1], roomOrient[2]);

	status("Loading room \"%s\" (%d)", roomFile.c_str(), _id);

	const GFF4List &models = rmlTop.getList(kGFF4EnvRoomModelList);
	_models.reserve(models.size());

	for (GFF4List::const_iterator m = models.begin(); m != models.end(); ++m) {
		if (!*m || ((*m)->getLabel() != kMDLID))
			continue;

		float scale = (*m)->getFloat(kGFF4EnvModelScale);

		float pos[3] = { 0.0f, 0.0f, 0.0f };
		(*m)->getVector3(kGFF4Position, pos[0], pos[1], pos[2]);

		float orient[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		(*m)->getVector4(kGFF4Orientation, orient[0], orient[1], orient[2], orient[3]);
		orient[3] = Common::rad2deg(acos(orient[3]) * 2.0);

		// TODO: Instances

		Graphics::Aurora::Model *model = loadModelObject((*m)->getString(kGFF4EnvModelFile));
		if (!model)
			continue;

		_models.push_back(model);

		Common::TransformationMatrix modelTransform(roomTransform);

		modelTransform.translate(pos[0], pos[1], pos[2]);
		modelTransform.rotate(orient[3], orient[0], orient[1], orient[2]);

		modelTransform.getPosition(pos[0], pos[1], pos[2]);
		modelTransform.getAxisAngle(orient[3], orient[0], orient[1], orient[2]);

		model->setPosition(pos[0], pos[1], pos[2]);
		model->setOrientation(orient[0], orient[1], orient[2], orient[3]);
		model->setScale(scale, scale, scale);
	}
}

void Room::show() {
	for (Models::iterator m = _models.begin(); m != _models.end(); ++m)
		(*m)->show();
}

void Room::hide() {
	for (Models::iterator m = _models.begin(); m != _models.end(); ++m)
		(*m)->hide();
}

} // End of namespace DragonAge

} // End of namespace Engines
