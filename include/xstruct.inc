typedef struct {
#define X_STRUCT_MEMBER(type,name) \
	type name;
X_STRUCT_MEMBERS
#undef X_STRUCT_MEMBER
} X_STRUCT_NAME;

