// project header files
#include "objectdata_studiolights.h"
#include "c4d_symbols.h"
#include "../../../microsdk/res/description/ostudiolights.h"

// classic API header files
#include "c4d.h"

#include "c4d_commandplugin.h"
#include "c4d_gui.h"
#include "c4d_general.h"
#include "c4d_basedocument.h"
#include "c4d_resource.h"
#include "c4d_basetag.h"

// MAXON API header files
#include "maxon/thread.h"


// Additional Includes 
#include "olight.h"
#include "oextrude.h"
#include "ttargetexpression.h"



#define PLUGIN_ID_STUDIOLIGHTSOBJECT 1000001 

namespace microsdk {

class StudioLightsObject : public ObjectData {

	INSTANCEOF(StudioLightsObject, ObjectData) 

	private:

		BaseObject* fillLight; // Olight object
		BaseObject* keyLight; // Olight object
		BaseObject* backLight; // Olight object
		BaseObject* camera; // Ocamera object
		BaseObject* stageSplineExtrude; // Oextrude object

		// Spline points for the stage
		Vector stageSplinePoints[3];
		// The spline that defines the stage curve
		SplineObject* stageSpline; 
		// The length of the stage extruded stageSpline
		Float stageLength;

		// Target tags
		BaseTag* fillLightTargetTag;
		BaseTag* keyLightTargetTag;
		BaseTag* backLightTargetTag;
		BaseTag* cameraTargetTag;

	public:
		static NodeData* Alloc() { return NewObj(StudioLightsObject) iferr_ignore("Error handled in C4D."); }

		virtual Bool Init(GeListNode* node);
		virtual Bool GetDEnabling(GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_ENABLE flags, const BaseContainer* itemdesc);
		virtual Bool GetDDescription(GeListNode* node, Description* description, DESCFLAGS_DESC& flags);
	};




	Bool StudioLightsObject::Init(GeListNode* node) {
		BaseContainer* bc = ((BaseList2D*)node)->GetDataInstance();

		// check for main thread
		if (!maxon::ThreadRef::IsMainThread())
			return false;

		BaseDocument* const doc = GetActiveDocument();
		if (doc == nullptr)
			return false;

		// Default Spline points
		stageSplinePoints[0] = Vector(0, 0, -500);
		stageSplinePoints[1] = Vector(0, 0, 500);
		stageSplinePoints[2] = Vector(0, 500, 500);

		// General Tab Defaults
		bc->SetLink(ID_STUDIOLIGHTS_GENERAL_TARGET_LINKBOX, nullptr);

		// Light Tab Defaults
		bc->SetBool(ID_STUDIOLIGHTS_LIGHT_TARGET_CHECKBOX, true);

		// Fill light defaults
		bc->SetBool(ID_STUDIOLIGHTS_LIGHT_FILLLIGHT_TOGGLE, true);
		bc->SetFloat(ID_STUDIOLIGHTS_LIGHT_FILLLIGHT_BRIGHTNESS, 100);
		bc->SetVector(ID_STUDIOLIGHTS_LIGHT_FILLLIGHT_COLOR, Vector(1, 1, 1));

		// Key light defaults
		bc->SetBool(ID_STUDIOLIGHTS_LIGHT_KEYLIGHT_TOGGLE, true);
		bc->SetFloat(ID_STUDIOLIGHTS_LIGHT_KEYLIGHT_BRIGHTNESS, 100);
		bc->SetVector(ID_STUDIOLIGHTS_LIGHT_KEYLIGHT_COLOR, Vector(1, 1, 1));

		// Back light defaults
		bc->SetBool(ID_STUDIOLIGHTS_LIGHT_BACKLIGHT_TOGGLE, true);
		bc->SetFloat(ID_STUDIOLIGHTS_LIGHT_BACKLIGHT_BRIGHTNESS, 100);
		bc->SetVector(ID_STUDIOLIGHTS_LIGHT_BACKLIGHT_COLOR, Vector(1, 1, 1));

		// Camera tab defaults
		bc->SetBool(ID_STUDIOLIGHTS_CAMERA_TARGET_CHECKBOX, true);

		// Stage tab defaults
		stageLength = 1000.0f;
		bc->SetFloat(ID_STUDIOLIGHTS_STAGE_LENGTH, stageLength);

		
		// Create stage objects
		fillLight = BaseObject::Alloc(Olight);
		keyLight = BaseObject::Alloc(Olight);
		backLight = BaseObject::Alloc(Olight);
		camera = BaseObject::Alloc(Ocamera);
		stageSplineExtrude = BaseObject::Alloc(Oextrude);

		// Create spline from spline points
		stageSpline = FitCurve(stageSplinePoints, 3, 1, nullptr);
		BaseContainer* stageSplineData = stageSpline->GetDataInstance();
		stageSplineData->SetFloat(SPLINEOBJECT_ANGLE, 0.0f);
		
		


		if (fillLight == nullptr
			|| keyLight == nullptr
			|| backLight == nullptr
			|| stageSplineExtrude == nullptr
			|| stageSpline == nullptr)
		{
			BaseObject::Free(fillLight);
			BaseObject::Free(keyLight);
			BaseObject::Free(backLight);
			BaseObject::Free(stageSplineExtrude);
			SplineObject::Free(stageSpline);

			return false;
		}



		// set names of stage objects
		
		fillLight->SetName("Fill Light"_s);
		keyLight->SetName("Key Light"_s);
		backLight->SetName("Back Light"_s);
		camera->SetName("Studio Camera"_s);
		stageSplineExtrude->SetName("Studio Stage"_s);
		stageSpline->SetName("Spline"_s);

		// Set extrude object's length
		stageSplineExtrude->GetDataInstance()->SetVector(EXTRUDEOBJECT_MOVE, Vector(stageLength, 0, 0));

		// Create a big undo for all the objects that are being inserted when this object is made
		doc->StartUndo();

		// Use the current node as the parent container for all our stage objects
		BaseObject* parentObject = static_cast<BaseObject*>(node);

		doc->InsertObject(fillLight, parentObject, nullptr);
		doc->InsertObject(keyLight, parentObject, nullptr);
		doc->InsertObject(backLight, parentObject, nullptr);
		doc->InsertObject(camera, parentObject, nullptr);
		doc->InsertObject(stageSplineExtrude, parentObject, nullptr);
		doc->InsertObject(stageSpline, stageSplineExtrude, nullptr);


		doc->AddUndo(UNDOTYPE::NEWOBJ, parentObject);
		doc->AddUndo(UNDOTYPE::NEWOBJ, fillLight);
		doc->AddUndo(UNDOTYPE::NEWOBJ, keyLight);
		doc->AddUndo(UNDOTYPE::NEWOBJ, backLight);
		doc->AddUndo(UNDOTYPE::NEWOBJ, camera);

		doc->AddUndo(UNDOTYPE::NEWOBJ, stageSplineExtrude);
		doc->AddUndo(UNDOTYPE::NEWOBJ, stageSpline);

		// Set the default positions for the stage objects
		// TODO: (steven) remove magic values
		fillLight->SetRelPos(Vector(300, 50, 0));
		keyLight->SetRelPos(Vector(-300, 50, 0));
		backLight->SetRelPos(Vector(-300, 50, 300));
		camera->SetRelPos(Vector(0, 0, -200));

		// Move the stage half way so that it is centered on the camera
		stageSplineExtrude->SetRelPos(Vector(-stageLength * 0.5f, 0, 0));
		stageSplineExtrude->GetDataInstance()->SetVector(EXTRUDEOBJECT_MOVE, Vector(stageLength, 0, 0));

		// Create target tags for the stage objects
		fillLightTargetTag = fillLight->MakeTag(Ttargetexpression);
		keyLightTargetTag = keyLight->MakeTag(Ttargetexpression);
		backLightTargetTag = backLight->MakeTag(Ttargetexpression);
		cameraTargetTag = camera->MakeTag(Ttargetexpression);

		if (fillLightTargetTag == nullptr
			|| keyLightTargetTag == nullptr
			|| backLightTargetTag == nullptr
			|| cameraTargetTag == nullptr)
		{
			return false;
		}

		doc->AddUndo(UNDOTYPE::NEWOBJ, fillLightTargetTag);
		doc->AddUndo(UNDOTYPE::NEWOBJ, keyLightTargetTag);
		doc->AddUndo(UNDOTYPE::NEWOBJ, backLightTargetTag);
		doc->AddUndo(UNDOTYPE::NEWOBJ, cameraTargetTag);

		
		fillLight->GetDataInstance()->SetInt32(LIGHT_TYPE, LIGHT_TYPE_AREA);
		keyLight->GetDataInstance()->SetInt32(LIGHT_TYPE, LIGHT_TYPE_AREA);
		backLight->GetDataInstance()->SetInt32(LIGHT_TYPE, LIGHT_TYPE_AREA);

		

		doc->EndUndo();



		EventAdd();

		return true;
	}


	Bool StudioLightsObject::GetDEnabling(GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_ENABLE flags, const BaseContainer* itemdesc) {
		return true;
	}

	Bool StudioLightsObject::GetDDescription(GeListNode* node, Description* description, DESCFLAGS_DESC& flags) {
		

		

		if (!description->LoadDescription(node->GetType()))
			return false;

		GeData data;
		GeListNode* thisNode = Get();
		BaseDocument* doc = GetActiveDocument();


		thisNode->GetParameter(ID_STUDIOLIGHTS_LIGHT_TARGET_CHECKBOX, data, DESCFLAGS_GET::NONE);
		Bool lightTargeetChecked = data.GetBool();


		thisNode->GetParameter(ID_STUDIOLIGHTS_CAMERA_TARGET_CHECKBOX, data, DESCFLAGS_GET::NONE);
		Bool cameraTargeetChecked = data.GetBool();

		// Get Target
		thisNode->GetParameter(DescID(ID_STUDIOLIGHTS_GENERAL_TARGET_LINKBOX), data, DESCFLAGS_GET::NONE);
		auto target = data.GetLink(GetActiveDocument());

		C4DAtomGoal* lightTarget = lightTargeetChecked ? target : nullptr;
		fillLightTargetTag->GetDataInstance()->SetLink(TARGETEXPRESSIONTAG_LINK, lightTarget);
		keyLightTargetTag->GetDataInstance()->SetLink(TARGETEXPRESSIONTAG_LINK, lightTarget);
		backLightTargetTag->GetDataInstance()->SetLink(TARGETEXPRESSIONTAG_LINK, lightTarget);
		
		
		C4DAtomGoal* cameraTarget = cameraTargeetChecked ? target : nullptr;
		cameraTargetTag->GetDataInstance()->SetLink(TARGETEXPRESSIONTAG_LINK, cameraTarget);

		
		thisNode->GetParameter(ID_STUDIOLIGHTS_STAGE_LENGTH, data, DESCFLAGS_GET::NONE);
		Float newExtrudeLength = data.GetFloat();
		stageLength = newExtrudeLength;


		
		stageSplineExtrude->SetRelPos(Vector(-stageLength * 0.5f, 0, 0));
		stageSplineExtrude->GetDataInstance()->SetVector(EXTRUDEOBJECT_MOVE, Vector(stageLength, 0, 0));
		
		


		// Fill Light parameters
		thisNode->GetParameter(ID_STUDIOLIGHTS_LIGHT_FILLLIGHT_TOGGLE, data, DESCFLAGS_GET::NONE);
		Bool fillLightChecked = data.GetBool();

		thisNode->GetParameter(ID_STUDIOLIGHTS_LIGHT_FILLLIGHT_BRIGHTNESS, data, DESCFLAGS_GET::NONE);
		Float fillLightBrightness = data.GetFloat();

		thisNode->GetParameter(ID_STUDIOLIGHTS_LIGHT_FILLLIGHT_COLOR, data, DESCFLAGS_GET::NONE);
		Vector fillLightColor = data.GetVector();

		BaseContainer* fillLightRef = fillLight->GetDataInstance();
		fillLightRef->SetBool(LIGHT_NOLIGHTRADIATION, fillLightChecked);
		fillLightRef->SetFloat(LIGHT_BRIGHTNESS, fillLightBrightness);
		fillLightRef->SetVector(LIGHT_COLOR, fillLightColor);



		// Key light parameters
		thisNode->GetParameter(ID_STUDIOLIGHTS_LIGHT_KEYLIGHT_TOGGLE, data, DESCFLAGS_GET::NONE);
		Bool keyLightChecked = data.GetBool();

		thisNode->GetParameter(ID_STUDIOLIGHTS_LIGHT_KEYLIGHT_BRIGHTNESS, data, DESCFLAGS_GET::NONE);
		Float keyLightBrightness = data.GetFloat();

		thisNode->GetParameter(ID_STUDIOLIGHTS_LIGHT_KEYLIGHT_COLOR, data, DESCFLAGS_GET::NONE);
		Vector keyLightColor = data.GetVector();

		BaseContainer* keyLightRef = keyLight->GetDataInstance();
		keyLightRef->SetBool(LIGHT_NOLIGHTRADIATION, keyLightChecked);
		keyLightRef->SetFloat(LIGHT_BRIGHTNESS, keyLightBrightness);
		keyLightRef->SetVector(LIGHT_COLOR, keyLightColor);


		// Back light parameters
		thisNode->GetParameter(ID_STUDIOLIGHTS_LIGHT_BACKLIGHT_TOGGLE, data, DESCFLAGS_GET::NONE);
		Bool backLightChecked = data.GetBool();

		thisNode->GetParameter(ID_STUDIOLIGHTS_LIGHT_BACKLIGHT_BRIGHTNESS, data, DESCFLAGS_GET::NONE);
		Float backLightBrightness = data.GetFloat();

		thisNode->GetParameter(ID_STUDIOLIGHTS_LIGHT_BACKLIGHT_COLOR, data, DESCFLAGS_GET::NONE);
		Vector backLightColor = data.GetVector();

		BaseContainer* backLightRef = backLight->GetDataInstance();
		backLightRef->SetBool(LIGHT_NOLIGHTRADIATION, backLightChecked);
		backLightRef->SetFloat(LIGHT_BRIGHTNESS, backLightBrightness);
		backLightRef->SetVector(LIGHT_COLOR, backLightColor);

		

		return SUPER::GetDDescription(node, description, flags);
	}




	void RegisterStudioLightsObject()
	{
		Bool success = RegisterObjectPlugin(PLUGIN_ID_STUDIOLIGHTSOBJECT, String("Studio Lights Object"), OBJECT_GENERATOR, StudioLightsObject::Alloc, "ostudiolights"_s, nullptr, 0);
		if (!success)
		{
			DiagnosticOutput("Could not register studio lights plugin.");
		}
	}
};