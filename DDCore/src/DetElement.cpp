//==========================================================================
//  AIDA Detector description implementation 
//--------------------------------------------------------------------------
// Copyright (C) Organisation europeenne pour la Recherche nucleaire (CERN)
// All rights reserved.
//
// For the licensing terms see $DD4hepINSTALL/LICENSE.
// For the list of contributors see $DD4hepINSTALL/doc/CREDITS.
//
// Author     : M.Frank
//
//==========================================================================

// Framework include files
#include "DD4hep/detail/DetectorInterna.h"
#include "DD4hep/detail/ConditionsInterna.h"
#include "DD4hep/detail/AlignmentsInterna.h"
#include "DD4hep/AlignmentTools.h"
#include "DD4hep/DetectorTools.h"
#include "DD4hep/Printout.h"
#include "DD4hep/World.h"
#include "DD4hep/Detector.h"

using namespace std;
using namespace dd4hep;
using dd4hep::Alignment;
    
namespace {
  static string s_empty_string;
}

/// Default constructor
DetElement::Processor::Processor()   {
}

/// Default destructor
DetElement::Processor::~Processor()   {
}

/// Clone constructor
DetElement::DetElement(Object* det_data, const string& det_name, const string& det_type)
  : Handle<DetElementObject>(det_data)
{
  this->assign(det_data, det_name, det_type);
}

/// Constructor for a new subdetector element
DetElement::DetElement(const string& det_name, const string& det_type, int det_id) {
  assign(new Object(det_name,det_id), det_name, det_type);
  ptr()->id = det_id;
}

/// Constructor for a new subdetector element
DetElement::DetElement(const string& det_name, int det_id) {
  assign(new Object(det_name,det_id), det_name, "");
  ptr()->id = det_id;
}

/// Constructor for a new subdetector element
DetElement::DetElement(DetElement det_parent, const string& det_name, int det_id) {
  assign(new Object(det_name,det_id), det_name, det_parent.type());
  ptr()->id = det_id;
  det_parent.add(*this);
}

/// Add an extension object to the detector element
void* DetElement::i_addExtension(void* ext_ptr, const type_info& info,
                                 void* (*ctor)(const void*, DetElement),
                                 void  (*dtor)(void*) ) const
{
  return access()->addExtension(ext_ptr, info, (void* (*)(const void*, void*))(ctor), dtor);
}

/// Access an existing extension object from the detector element
void* DetElement::i_extension(const type_info& info) const {
  return access()->extension(info);
}

/// Internal call to extend the detector element with an arbitrary structure accessible by the type
void DetElement::i_addUpdateCall(unsigned int callback_type, const Callback& callback)  const  {
  access()->updateCalls.push_back(make_pair(callback,callback_type));
}

/// Remove callback from object
void DetElement::removeAtUpdate(unsigned int typ, void* pointer)  const {
  access()->removeAtUpdate(typ,pointer);
}

/// Access to the full path to the placed object
const string& DetElement::placementPath() const {
  Object* o = ptr();
  if ( o ) {
    if (o->placementPath.empty()) {
      o->placementPath = detail::tools::placementPath(*this);
    }
    return o->placementPath;
  }
  return s_empty_string;
}

/// Access detector type (structure, tracker, calorimeter, etc.).
string DetElement::type() const {
  return m_element ? m_element->GetTitle() : "";
}

/// Set the type of the sensitive detector
DetElement& DetElement::setType(const string& typ) {
  access()->SetTitle(typ.c_str());
  return *this;
}

unsigned int DetElement::typeFlag() const {
  return m_element ? m_element->typeFlag :  0 ;
}

/// Set the type of the sensitive detector
DetElement& DetElement::setTypeFlag(unsigned int types) {
  access()->typeFlag = types ;
  return *this;
}

namespace {
  static void make_path(DetElement::Object* o)   {
    DetElement par = o->parent;
    if ( par.isValid() )  {
      o->path = par.path() + "/" + o->name;
      if ( o->level < 0 ) o->level = par.level() + 1;
    }
    else {
      o->path = "/" + o->name;
      o->level = 0;
    }
    o->key = dd4hep::detail::hash32(o->path);
  }
}

/// Access hash key of this detector element (Only valid once geometry is closed!)
unsigned int DetElement::key()  const   {
  Object* o = ptr();
  if ( o )  {
    if ( o->key != 0 )
      return o->key;
    make_path(o);
    return o->key;
  }
  return 0;
}

/// Access the hierarchical level of the detector element
int DetElement::level()  const   {
  Object* o = ptr();
  if ( o )  {
    if ( o->level >= 0 )
      return o->level;
    make_path(o);
    return o->level;
  }
  return -1;
}

/// Access the full path of the detector element
const string& DetElement::path() const {
  Object* o = ptr();
  if ( o ) {
    if ( !o->path.empty() )
      return o->path;
    make_path(o);
    return o->path;
  }
  return s_empty_string;
}

int DetElement::id() const {
  return access()->id;
}

bool DetElement::combineHits() const {
  return access()->combineHits != 0;
}

DetElement& DetElement::setCombineHits(bool value, SensitiveDetector& sens) {
  access()->combineHits = value;
  if (sens.isValid())
    sens.setCombineHits(value);
  return *this;
}

/// Access to the alignment information
Alignment DetElement::nominal() const   {
  Object* o = access();
  if ( !o->nominal.isValid() )   {
    o->nominal = AlignmentCondition("nominal");
    o->nominal->values().detector = *this;
    //o->flag |= Object::HAVE_WORLD_TRAFO;
    //o->flag |= Object::HAVE_PARENT_TRAFO;
    dd4hep::detail::tools::computeIdeal(o->nominal);
  }
  return o->nominal;
}

/// Access to the survey alignment information
Alignment DetElement::survey() const  {
  Object* o = access();
  if ( !o->survey.isValid() )   {
    o->survey = AlignmentCondition("survey");
    dd4hep::detail::tools::copy(nominal(), o->survey);
  }
  return o->survey;
}

const DetElement::Children& DetElement::children() const {
  return access()->children;
}

/// Access to individual children by name
DetElement DetElement::child(const string& child_name) const {
  if (isValid()) {
    const Children& c = ptr()->children;
    Children::const_iterator i = c.find(child_name);
    return i == c.end() ? DetElement() : (*i).second;
  }
  return DetElement();
}

/// Access to the detector elements's parent
DetElement DetElement::parent() const {
  Object* o = ptr();
  return (o) ? o->parent : 0;
}

/// Access to the world object. Only possible once the geometry is closed.
DetElement DetElement::world()  const   {
  Object* o = ptr();
  return (o) ? o->world() : 0;
}

void DetElement::check(bool cond, const string& msg) const {
  if (cond) {
    throw runtime_error("dd4hep: " + msg);
  }
}

DetElement& DetElement::add(DetElement sdet) {
  if (isValid()) {
    pair<Children::iterator, bool> r = object<Object>().children.insert(make_pair(sdet.name(), sdet));
    if (r.second) {
      sdet.access()->parent = *this;
      return *this;
    }
    throw runtime_error("dd4hep: DetElement::add: Element " + string(sdet.name()) + 
                        " is already present in path " + path() + " [Double-Insert]");
  }
  throw runtime_error("dd4hep: DetElement::add: Self is not defined [Invalid Handle]");
}

DetElement DetElement::clone(const string& new_name) const {
  Object* o = access();
  return DetElement(o->clone(o->id, COPY_NONE), new_name, o->GetTitle());
}

DetElement DetElement::clone(const string& new_name, int new_id) const {
  Object* o = access();
  return DetElement(o->clone(new_id, COPY_NONE), new_name, o->GetTitle());
}

/// Access to the ideal physical volume of this detector element
PlacedVolume DetElement::idealPlacement() const    {
  if (isValid()) {
    Object& o = object<Object>();
    return o.idealPlace;
  }
  return PlacedVolume();
}

/// Access to the physical volume of this detector element
PlacedVolume DetElement::placement() const {
  if (isValid()) {
    Object& o = object<Object>();
    return o.placement;
  }
  return PlacedVolume();
}

/// Set the physical volumes of the detector element
DetElement& DetElement::setPlacement(const PlacedVolume& pv) {
  if (pv.isValid()) {
    Object* o = access();
    o->placement = pv;
    if ( !o->idealPlace.isValid() )  {
      o->idealPlace = pv;
    }
    return *this;
  }
  throw runtime_error("dd4hep: DetElement::setPlacement: Placement is not defined [Invalid Handle]");
}

/// The cached VolumeID of this subdetector element
dd4hep::VolumeID DetElement::volumeID() const   {
  if (isValid()) {
    return object<Object>().volumeID;
  }
  return 0;
}

/// Access to the logical volume of the placements (all daughters have the same!)
Volume DetElement::volume() const {
  return access()->placement.volume();
}

DetElement& DetElement::setVisAttributes(const Detector& description, const string& nam, const Volume& vol) {
  vol.setVisAttributes(description, nam);
  return *this;
}

DetElement& DetElement::setRegion(const Detector& description, const string& nam, const Volume& vol) {
  if (!nam.empty()) {
    vol.setRegion(description.region(nam));
  }
  return *this;
}

DetElement& DetElement::setLimitSet(const Detector& description, const string& nam, const Volume& vol) {
  if (!nam.empty()) {
    vol.setLimitSet(description.limitSet(nam));
  }
  return *this;
}

DetElement& DetElement::setAttributes(const Detector& description,
                                      const Volume& vol,
                                      const string& region,
                                      const string& limits,
                                      const string& vis)
{
  return setRegion(description, region, vol).setLimitSet(description, limits, vol).setVisAttributes(description, vis, vol);
}

/// Constructor
SensitiveDetector::SensitiveDetector(const string& nam, const string& typ) {
  /*
    <calorimeter ecut="0" eunit="MeV" hits_collection="EcalEndcapHits" name="EcalEndcap" verbose="0">
    <global_grid_xy grid_size_x="3.5" grid_size_y="3.5"/>
    <idspecref ref="EcalEndcapHits"/>
    </calorimeter>
  */
  assign(new Object(nam), nam, typ);
  object<Object>().ecut = 0e0;
  object<Object>().verbose = 0;
}

/// Set the type of the sensitive detector
SensitiveDetector& SensitiveDetector::setType(const string& typ) {
  access()->SetTitle(typ.c_str());
  return *this;
}

/// Access the type of the sensitive detector
string SensitiveDetector::type() const {
  return m_element ? m_element->GetTitle() : s_empty_string;
}

/// Assign the IDDescriptor reference
SensitiveDetector& SensitiveDetector::setReadout(Readout ro) {
  access()->readout = ro;
  return *this;
}

/// Assign the IDDescriptor reference
Readout SensitiveDetector::readout() const {
  return access()->readout;
}

/// Set energy cut off
SensitiveDetector& SensitiveDetector::setEnergyCutoff(double value) {
  access()->ecut = value;
  return *this;
}

/// Access energy cut off
double SensitiveDetector::energyCutoff() const {
  return access()->ecut;
}

/// Assign the name of the hits collection
SensitiveDetector& SensitiveDetector::setHitsCollection(const string& collection) {
  access()->hitsCollection = collection;
  return *this;
}

/// Access the hits collection name
const string& SensitiveDetector::hitsCollection() const {
  return access()->hitsCollection;
}

/// Assign the name of the hits collection
SensitiveDetector& SensitiveDetector::setVerbose(bool value) {
  int v = value ? 1 : 0;
  access()->verbose = v;
  return *this;
}

/// Access flag to combine hist
bool SensitiveDetector::verbose() const {
  return access()->verbose == 1;
}

/// Assign the name of the hits collection
SensitiveDetector& SensitiveDetector::setCombineHits(bool value) {
  int v = value ? 1 : 0;
  access()->combineHits = v;
  return *this;
}

/// Access flag to combine hist
bool SensitiveDetector::combineHits() const {
  return access()->combineHits == 1;
}

/// Set the regional attributes to the sensitive detector
SensitiveDetector& SensitiveDetector::setRegion(Region reg) {
  access()->region = reg;
  return *this;
}

/// Access to the region setting of the sensitive detector (not mandatory)
Region SensitiveDetector::region() const {
  return access()->region;
}

/// Set the limits to the sensitive detector
SensitiveDetector& SensitiveDetector::setLimitSet(LimitSet ls) {
  access()->limits = ls;
  return *this;
}

/// Access to the limit set of the sensitive detector (not mandatory).
LimitSet SensitiveDetector::limits() const {
  return access()->limits;
}

/// Add an extension object to the detector element
void* SensitiveDetector::i_addExtension(void* ext_ptr,
                                        const type_info& info,
                                        void (*dtor)(void*))
{
  return access()->addExtension(ext_ptr, info, dtor);
}

/// Access an existing extension object from the detector element
void* SensitiveDetector::i_extension(const type_info& info) const {
  return access()->extension(info);
}
