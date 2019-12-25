/*
 * tuning.h
 * --------
 * Purpose: Alternative sample tuning.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include <map>

#include "tuningbase.h"


OPENMPT_NAMESPACE_BEGIN


namespace Tuning {


class CTuning
{

public:

	static constexpr char s_FileExtension[5] = ".tun";

	static constexpr RATIOTYPE s_DefaultFallbackRatio = 1.0f;
	static constexpr NOTEINDEXTYPE s_NoteMinDefault = -64;
	static constexpr UNOTEINDEXTYPE s_RatioTableSizeDefault = 128;
	static constexpr USTEPINDEXTYPE s_RatioTableFineSizeMaxDefault = 1000;

public:

	// To return ratio of certain note.
	RATIOTYPE GetRatio(const NOTEINDEXTYPE note) const;

	// To return ratio from a 'step'(noteindex + stepindex)
	RATIOTYPE GetRatio(const NOTEINDEXTYPE baseNote, const STEPINDEXTYPE baseFineSteps) const;

	//Tuning might not be valid for arbitrarily large range,
	//so this can be used to ask where it is valid. Tells the lowest and highest
	//note that are valid.
	MPT_FORCEINLINE NoteRange GetNoteRange() const
	{
		return NoteRange{m_NoteMin, static_cast<NOTEINDEXTYPE>(m_NoteMin + static_cast<NOTEINDEXTYPE>(m_RatioTable.size()) - 1)};
	}

	// Return true if note is within note range
	MPT_FORCEINLINE bool IsValidNote(const NOTEINDEXTYPE n) const
	{
		return (GetNoteRange().first <= n && n <= GetNoteRange().last);
	}

	MPT_FORCEINLINE UNOTEINDEXTYPE GetGroupSize() const
	{
		return m_GroupSize;
	}

	RATIOTYPE GetGroupRatio() const {return m_GroupRatio;}

	// To return (fine)stepcount between two consecutive mainsteps.
	MPT_FORCEINLINE USTEPINDEXTYPE GetFineStepCount() const
	{
		return m_FineStepCount;
	}

	//To return 'directed distance' between given notes.
	STEPINDEXTYPE GetStepDistance(const NOTEINDEXTYPE& from, const NOTEINDEXTYPE& to) const
		{return (to - from)*(static_cast<NOTEINDEXTYPE>(GetFineStepCount())+1);}

	//To return 'directed distance' between given steps.
	STEPINDEXTYPE GetStepDistance(const NOTEINDEXTYPE& noteFrom, const STEPINDEXTYPE& stepDistFrom, const NOTEINDEXTYPE& noteTo, const STEPINDEXTYPE& stepDistTo) const
		{return GetStepDistance(noteFrom, noteTo) + stepDistTo - stepDistFrom;}

	//To set finestepcount between two consecutive mainsteps.
	//Finestep count == 0 means that
	//stepdistances become the same as note distances.
	void SetFineStepCount(const USTEPINDEXTYPE& fs);

	//Multiply all ratios by given number.
	bool Multiply(const RATIOTYPE r);

	bool SetRatio(const NOTEINDEXTYPE& s, const RATIOTYPE& r);

	MPT_FORCEINLINE Tuning::Type GetType() const
	{
		return m_TuningType;
	}

	std::string GetNoteName(const NOTEINDEXTYPE& x, bool addOctave = true) const;

	void SetNoteName(const NOTEINDEXTYPE&, const std::string&);

	static std::unique_ptr<CTuning> CreateDeserialize(std::istream & f)
	{
		std::unique_ptr<CTuning> pT = std::unique_ptr<CTuning>(new CTuning());
		if(pT->InitDeserialize(f) != SerializationResult::Success)
		{
			return nullptr;
		}
		return pT;
	}

	//Try to read old version (v.3) and return pointer to new instance if succesfull, else nullptr.
	static std::unique_ptr<CTuning> CreateDeserializeOLD(std::istream & f)
	{
		std::unique_ptr<CTuning> pT = std::unique_ptr<CTuning>(new CTuning());
		if(pT->InitDeserializeOLD(f) != SerializationResult::Success)
		{
			return nullptr;
		}
		return pT;
	}

	static std::unique_ptr<CTuning> CreateGeneral(const std::string &name)
	{
		std::unique_ptr<CTuning> pT = std::unique_ptr<CTuning>(new CTuning());
		pT->SetName(name);
		return pT;
	}

	static std::unique_ptr<CTuning> CreateGroupGeometric(const std::string &name, UNOTEINDEXTYPE groupsize, RATIOTYPE groupratio, USTEPINDEXTYPE finestepcount)
	{
		std::unique_ptr<CTuning> pT = std::unique_ptr<CTuning>(new CTuning());
		pT->SetName(name);
		if(pT->CreateGroupGeometric(groupsize, groupratio, 0) != false)
		{
			return nullptr;
		}
		pT->SetFineStepCount(finestepcount);
		return pT;
	}

	static std::unique_ptr<CTuning> CreateGroupGeometric(const std::string &name, const std::vector<RATIOTYPE> &ratios, RATIOTYPE groupratio, USTEPINDEXTYPE finestepcount)
	{
		std::unique_ptr<CTuning> pT = std::unique_ptr<CTuning>(new CTuning());
		pT->SetName(name);
		NoteRange range = NoteRange{s_NoteMinDefault, static_cast<NOTEINDEXTYPE>(s_NoteMinDefault + s_RatioTableSizeDefault - 1)};
		range.last = std::max(range.last, mpt::saturate_cast<NOTEINDEXTYPE>(ratios.size() - 1));
		range.first = 0 - range.last - 1;
		if(pT->CreateGroupGeometric(ratios, groupratio, range, 0) != false)
		{
			return nullptr;
		}
		pT->SetFineStepCount(finestepcount);
		return pT;
	}

	static std::unique_ptr<CTuning> CreateGeometric(const std::string &name, UNOTEINDEXTYPE groupsize, RATIOTYPE groupratio, USTEPINDEXTYPE finestepcount)
	{
		std::unique_ptr<CTuning> pT = std::unique_ptr<CTuning>(new CTuning());
		pT->SetName(name);
		if(pT->CreateGeometric(groupsize, groupratio) != false)
		{
			return nullptr;
		}
		pT->SetFineStepCount(finestepcount);
		return pT;
	}

	Tuning::SerializationResult Serialize(std::ostream& out) const;

#ifdef MODPLUG_TRACKER
	bool WriteSCL(std::ostream &f, const mpt::PathString &filename) const;
#endif

	bool ChangeGroupsize(const NOTEINDEXTYPE&);
	bool ChangeGroupRatio(const RATIOTYPE&);

	void SetName(const std::string& s) { m_TuningName = s; }
	std::string GetName() const {return m_TuningName;}

private:

	CTuning();

	SerializationResult InitDeserialize(std::istream& inStrm);

	//Try to read old version (v.3) and return pointer to new instance if succesfull, else nullptr.
	SerializationResult InitDeserializeOLD(std::istream&);

	//Create GroupGeometric tuning of *this using virtual ProCreateGroupGeometric.
	bool CreateGroupGeometric(const std::vector<RATIOTYPE> &v, const RATIOTYPE &r, const NoteRange &range, const NOTEINDEXTYPE &ratiostartpos);

	//Create GroupGeometric of *this using ratios from 'itself' and ratios starting from
	//position given as third argument.
	bool CreateGroupGeometric(const NOTEINDEXTYPE &s, const RATIOTYPE &r, const NOTEINDEXTYPE &startindex);

	//Create geometric tuning of *this using ratio(0) = 1.
	bool CreateGeometric(const UNOTEINDEXTYPE &p, const RATIOTYPE &r);
	bool CreateGeometric(const UNOTEINDEXTYPE &s, const RATIOTYPE &r, const NoteRange &range);

	void UpdateFineStepTable();

	// GroupPeriodic-specific.
	// Get the corresponding note in [0, period-1].
	// For example GetRefNote(-1) is to return note :'groupsize-1'.
	MPT_FORCEINLINE NOTEINDEXTYPE GetRefNote(NOTEINDEXTYPE note) const
	{
		MPT_ASSERT(GetType() == Type::GROUPGEOMETRIC || GetType() == Type::GEOMETRIC);
		return static_cast<NOTEINDEXTYPE>(mpt::wrapping_modulo(note, GetGroupSize()));
	}

	static bool IsValidRatio(RATIOTYPE ratio)
	{
		return (ratio > static_cast<RATIOTYPE>(0.0));
	}

private:

	Tuning::Type m_TuningType;

	//Noteratios
	std::vector<RATIOTYPE> m_RatioTable;

	//'Fineratios'
	std::vector<RATIOTYPE> m_RatioTableFine;

	// The lowest index of note in the table
	NOTEINDEXTYPE m_NoteMin;

	//For groupgeometric tunings, tells the 'group size' and 'group ratio'
	//m_GroupSize should always be >= 0.
	NOTEINDEXTYPE m_GroupSize;
	RATIOTYPE m_GroupRatio;

	USTEPINDEXTYPE m_FineStepCount; // invariant: 0 <= m_FineStepCount <= FINESTEPCOUNT_MAX

	std::string m_TuningName;

	std::map<NOTEINDEXTYPE, std::string> m_NoteNameMap;

}; // class CTuning


} // namespace Tuning


OPENMPT_NAMESPACE_END
