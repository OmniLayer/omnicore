#ifndef OMNICORE_RPC_H
#define OMNICORE_RPC_H

#include "omnicore/mdex.h"

/** Throws a JSONRPCError, depending on error code. */
void PopulateFailure(int error);

void MetaDexObjectToJSON(const CMPMetaDEx& obj, json_spirit::Object& metadex_obj);

#endif // OMNICORE_RPC_H
