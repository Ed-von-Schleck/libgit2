#include "clar_libgit2.h"

#include "git2/revparse.h"
#include "buffer.h"
#include "refs.h"
#include "path.h"

static git_repository *g_repo;
static git_object *g_obj;
static char g_orig_tz[16] = {0};



/* Helpers */
static void test_object_inrepo(const char *spec, const char *expected_oid, git_repository *repo)
{
	char objstr[64] = {0};
	git_object *obj = NULL;
	int error;

	error = git_revparse_single(&obj, repo, spec);

	if (expected_oid != NULL) {
		cl_assert_equal_i(0, error);
		git_oid_fmt(objstr, git_object_id(obj));
		cl_assert_equal_s(objstr, expected_oid);
	} else
		cl_assert_equal_i(GIT_ENOTFOUND, error);

	git_object_free(obj);
}

static void test_object(const char *spec, const char *expected_oid)
{
	test_object_inrepo(spec, expected_oid, g_repo);
}

void test_refs_revparse__initialize(void)
{
	char *tz = cl_getenv("TZ");
	if (tz)
		strcpy(g_orig_tz, tz);
	cl_setenv("TZ", "UTC");

	cl_git_pass(git_repository_open(&g_repo, cl_fixture("testrepo.git")));
}

void test_refs_revparse__cleanup(void)
{
	git_repository_free(g_repo);
	g_obj = NULL;
	cl_setenv("TZ", g_orig_tz);
}

void test_refs_revparse__nonexistant_object(void)
{
	test_object("this-does-not-exist", NULL);
	test_object("this-does-not-exist^1", NULL);
	test_object("this-does-not-exist~2", NULL);
}

void test_refs_revparse__invalid_reference_name(void)
{
	cl_git_fail(git_revparse_single(&g_obj, g_repo, "this doesn't make sense"));
	cl_git_fail(git_revparse_single(&g_obj, g_repo, "this doesn't make sense^1"));
	cl_git_fail(git_revparse_single(&g_obj, g_repo, "this doesn't make sense~2"));
	cl_git_fail(git_revparse_single(&g_obj, g_repo, ""));

}

void test_refs_revparse__shas(void)
{
	test_object("c47800c7266a2be04c571c04d5a6614691ea99bd", "c47800c7266a2be04c571c04d5a6614691ea99bd");
	test_object("c47800c", "c47800c7266a2be04c571c04d5a6614691ea99bd");
}

void test_refs_revparse__head(void)
{
	test_object("HEAD", "a65fedf39aefe402d3bb6e24df4d4f5fe4547750");
	test_object("HEAD^0", "a65fedf39aefe402d3bb6e24df4d4f5fe4547750");
	test_object("HEAD~0", "a65fedf39aefe402d3bb6e24df4d4f5fe4547750");
	test_object("master", "a65fedf39aefe402d3bb6e24df4d4f5fe4547750");
}

void test_refs_revparse__full_refs(void)
{
	test_object("refs/heads/master", "a65fedf39aefe402d3bb6e24df4d4f5fe4547750");
	test_object("refs/heads/test", "e90810b8df3e80c413d903f631643c716887138d");
	test_object("refs/tags/test", "b25fa35b38051e4ae45d4222e795f9df2e43f1d1");
}

void test_refs_revparse__partial_refs(void)
{
	test_object("point_to_blob", "1385f264afb75a56a5bec74243be9b367ba4ca08");
	test_object("packed-test", "4a202b346bb0fb0db7eff3cffeb3c70babbd2045");
	test_object("br2", "a4a7dce85cf63874e984719f4fdd239f5145052f");
}

void test_refs_revparse__describe_output(void)
{
	test_object("blah-7-gc47800c", "c47800c7266a2be04c571c04d5a6614691ea99bd");
	test_object("not-good", "a65fedf39aefe402d3bb6e24df4d4f5fe4547750");
}

void test_refs_revparse__nth_parent(void)
{
	cl_git_fail(git_revparse_single(&g_obj, g_repo, "be3563a^-1"));
	cl_git_fail(git_revparse_single(&g_obj, g_repo, "^"));
	cl_git_fail(git_revparse_single(&g_obj, g_repo, "be3563a^{tree}^"));

	test_object("be3563a^1", "9fd738e8f7967c078dceed8190330fc8648ee56a");
	test_object("be3563a^", "9fd738e8f7967c078dceed8190330fc8648ee56a");
	test_object("be3563a^2", "c47800c7266a2be04c571c04d5a6614691ea99bd");
	test_object("be3563a^1^1", "4a202b346bb0fb0db7eff3cffeb3c70babbd2045");
	test_object("be3563a^^", "4a202b346bb0fb0db7eff3cffeb3c70babbd2045");
	test_object("be3563a^2^1", "5b5b025afb0b4c913b4c338a42934a3863bf3644");
	test_object("be3563a^0", "be3563ae3f795b2b4353bcce3a527ad0a4f7f644");
	test_object("be3563a^{commit}^", "9fd738e8f7967c078dceed8190330fc8648ee56a");

	test_object("be3563a^42", NULL);
}

void test_refs_revparse__not_tag(void)
{
	test_object("point_to_blob^{}", "1385f264afb75a56a5bec74243be9b367ba4ca08");
	test_object("wrapped_tag^{}", "a65fedf39aefe402d3bb6e24df4d4f5fe4547750");
	test_object("master^{}", "a65fedf39aefe402d3bb6e24df4d4f5fe4547750");
	test_object("master^{tree}^{}", "944c0f6e4dfa41595e6eb3ceecdb14f50fe18162");
	test_object("e90810b^{}", "e90810b8df3e80c413d903f631643c716887138d");
	test_object("tags/e90810b^{}", "e90810b8df3e80c413d903f631643c716887138d");
	test_object("e908^{}", "e90810b8df3e80c413d903f631643c716887138d");
}

void test_refs_revparse__to_type(void)
{
	cl_git_fail(git_revparse_single(&g_obj, g_repo, "wrapped_tag^{blob}"));
	cl_git_fail(git_revparse_single(&g_obj, g_repo, "wrapped_tag^{trip}"));

	test_object("wrapped_tag^{commit}", "a65fedf39aefe402d3bb6e24df4d4f5fe4547750");
	test_object("wrapped_tag^{tree}", "944c0f6e4dfa41595e6eb3ceecdb14f50fe18162");
	test_object("point_to_blob^{blob}", "1385f264afb75a56a5bec74243be9b367ba4ca08");
	test_object("master^{commit}^{commit}", "a65fedf39aefe402d3bb6e24df4d4f5fe4547750");
}

void test_refs_revparse__linear_history(void)
{
	cl_git_fail(git_revparse_single(&g_obj, g_repo, "~"));
	cl_git_fail(git_revparse_single(&g_obj, g_repo, "foo~bar"));
	cl_git_fail(git_revparse_single(&g_obj, g_repo, "master~bar"));
	cl_git_fail(git_revparse_single(&g_obj, g_repo, "master~-1"));
	cl_git_fail(git_revparse_single(&g_obj, g_repo, "master~0bar"));

	test_object("master~0", "a65fedf39aefe402d3bb6e24df4d4f5fe4547750");
	test_object("master~1", "be3563ae3f795b2b4353bcce3a527ad0a4f7f644");
	test_object("master~2", "9fd738e8f7967c078dceed8190330fc8648ee56a");
	test_object("master~1~1", "9fd738e8f7967c078dceed8190330fc8648ee56a");
	test_object("master~~", "9fd738e8f7967c078dceed8190330fc8648ee56a");
}

void test_refs_revparse__chaining(void)
{
	cl_git_fail(git_revparse_single(&g_obj, g_repo, "master@{0}@{0}"));
	cl_git_fail(git_revparse_single(&g_obj, g_repo, "@{u}@{-1}"));
	cl_git_fail(git_revparse_single(&g_obj, g_repo, "@{-1}@{-1}"));
	cl_git_fail(git_revparse_single(&g_obj, g_repo, "@{-3}@{0}"));

	test_object("master@{0}~1^1", "9fd738e8f7967c078dceed8190330fc8648ee56a");
	test_object("@{u}@{0}", "be3563ae3f795b2b4353bcce3a527ad0a4f7f644");
	test_object("@{-1}@{0}", "a4a7dce85cf63874e984719f4fdd239f5145052f");
	test_object("@{-4}@{1}", "be3563ae3f795b2b4353bcce3a527ad0a4f7f644");
	test_object("master~1^1", "9fd738e8f7967c078dceed8190330fc8648ee56a");
	test_object("master~1^2", "c47800c7266a2be04c571c04d5a6614691ea99bd");
	test_object("master^1^2~1", "5b5b025afb0b4c913b4c338a42934a3863bf3644");
	test_object("master^^2^", "5b5b025afb0b4c913b4c338a42934a3863bf3644");
	test_object("master^1^1^1^1^1", "8496071c1b46c854b31185ea97743be6a8774479");
	test_object("master^^1^2^1", NULL);
}

void test_refs_revparse__upstream(void)
{
	cl_git_fail(git_revparse_single(&g_obj, g_repo, "e90810b@{u}"));
	cl_git_fail(git_revparse_single(&g_obj, g_repo, "refs/tags/e90810b@{u}"));

	test_object("master@{upstream}", "be3563ae3f795b2b4353bcce3a527ad0a4f7f644");
	test_object("@{u}", "be3563ae3f795b2b4353bcce3a527ad0a4f7f644");
	test_object("master@{u}", "be3563ae3f795b2b4353bcce3a527ad0a4f7f644");
	test_object("heads/master@{u}", "be3563ae3f795b2b4353bcce3a527ad0a4f7f644");
	test_object("refs/heads/master@{u}", "be3563ae3f795b2b4353bcce3a527ad0a4f7f644");
}

void test_refs_revparse__ordinal(void)
{
	cl_git_fail(git_revparse_single(&g_obj, g_repo, "master@{-2}"));
	
	/* TODO: make the test below actually fail
	 * cl_git_fail(git_revparse_single(&g_obj, g_repo, "master@{1a}"));
	 */

	test_object("nope@{0}", NULL);
	test_object("master@{31415}", NULL);
	test_object("@{1000}", NULL);
	test_object("@{2}", NULL);

	test_object("@{0}", "a65fedf39aefe402d3bb6e24df4d4f5fe4547750");
	test_object("@{1}", "be3563ae3f795b2b4353bcce3a527ad0a4f7f644");

	test_object("master@{0}", "a65fedf39aefe402d3bb6e24df4d4f5fe4547750");
	test_object("master@{1}", "be3563ae3f795b2b4353bcce3a527ad0a4f7f644");
	test_object("heads/master@{1}", "be3563ae3f795b2b4353bcce3a527ad0a4f7f644");
	test_object("refs/heads/master@{1}", "be3563ae3f795b2b4353bcce3a527ad0a4f7f644");
}

void test_refs_revparse__previous_head(void)
{
	cl_git_fail(git_revparse_single(&g_obj, g_repo, "@{-xyz}"));
	cl_git_fail(git_revparse_single(&g_obj, g_repo, "@{-0}"));
	cl_git_fail(git_revparse_single(&g_obj, g_repo, "@{-1b}"));

	test_object("@{-42}", NULL);

	test_object("@{-2}", "a65fedf39aefe402d3bb6e24df4d4f5fe4547750");
	test_object("@{-1}", "a4a7dce85cf63874e984719f4fdd239f5145052f");
}

static void create_fake_stash_reference_and_reflog(git_repository *repo)
{
	git_reference *master;
	git_buf log_path = GIT_BUF_INIT;

	git_buf_joinpath(&log_path, git_repository_path(repo), "logs/refs/fakestash");

	cl_assert_equal_i(false, git_path_isfile(git_buf_cstr(&log_path)));

	cl_git_pass(git_reference_lookup(&master, repo, "refs/heads/master"));
	cl_git_pass(git_reference_rename(master, "refs/fakestash", 0));

	cl_assert_equal_i(true, git_path_isfile(git_buf_cstr(&log_path)));

	git_buf_free(&log_path);
	git_reference_free(master);
}

void test_refs_revparse__reflog_of_a_ref_under_refs(void)
{
	git_repository *repo = cl_git_sandbox_init("testrepo.git");

	test_object_inrepo("refs/fakestash", NULL, repo);

	create_fake_stash_reference_and_reflog(repo);

	/*
	 * $ git reflog -1 refs/fakestash
	 * a65fedf refs/fakestash@{0}: commit: checking in
	 *
	 * $ git reflog -1 refs/fakestash@{0}
	 * a65fedf refs/fakestash@{0}: commit: checking in
	 *
	 * $ git reflog -1 fakestash
	 * a65fedf fakestash@{0}: commit: checking in
	 *
	 * $ git reflog -1 fakestash@{0}
	 * a65fedf fakestash@{0}: commit: checking in
	 */
	test_object_inrepo("refs/fakestash", "a65fedf39aefe402d3bb6e24df4d4f5fe4547750", repo);
	test_object_inrepo("refs/fakestash@{0}", "a65fedf39aefe402d3bb6e24df4d4f5fe4547750", repo);
	test_object_inrepo("fakestash", "a65fedf39aefe402d3bb6e24df4d4f5fe4547750", repo);
	test_object_inrepo("fakestash@{0}", "a65fedf39aefe402d3bb6e24df4d4f5fe4547750", repo);

	cl_git_sandbox_cleanup();
}

void test_refs_revparse__revwalk(void)
{
	cl_git_fail(git_revparse_single(&g_obj, g_repo, "master^{/not found in any commit}"));
	cl_git_fail(git_revparse_single(&g_obj, g_repo, "master^{/merge}"));
	cl_git_fail(git_revparse_single(&g_obj, g_repo, "master^{/((}"));

	test_object("master^{/anoth}", "5b5b025afb0b4c913b4c338a42934a3863bf3644");
	test_object("master^{/Merge}", "be3563ae3f795b2b4353bcce3a527ad0a4f7f644");
	test_object("br2^{/Merge}", "a4a7dce85cf63874e984719f4fdd239f5145052f");
	test_object("master^{/fo.rth}", "9fd738e8f7967c078dceed8190330fc8648ee56a");
}

void test_refs_revparse__date(void)
{
	/*
	 * $ git reflog HEAD --date=iso
	 * a65fedf HEAD@{2012-04-30 08:23:41 -0900}: checkout: moving from br2 to master
	 * a4a7dce HEAD@{2012-04-30 08:23:37 -0900}: commit: checking in
	 * c47800c HEAD@{2012-04-30 08:23:28 -0900}: checkout: moving from master to br2
	 * a65fedf HEAD@{2012-04-30 08:23:23 -0900}: commit:
	 * be3563a HEAD@{2012-04-30 10:22:43 -0700}: clone: from /Users/ben/src/libgit2/tes
	 *
	 * $ git reflog HEAD --date=raw
	 * a65fedf HEAD@{1335806621 -0900}: checkout: moving from br2 to master
	 * a4a7dce HEAD@{1335806617 -0900}: commit: checking in
	 * c47800c HEAD@{1335806608 -0900}: checkout: moving from master to br2
	 * a65fedf HEAD@{1335806603 -0900}: commit:
	 * be3563a HEAD@{1335806563 -0700}: clone: from /Users/ben/src/libgit2/tests/resour
	 */
	test_object("HEAD@{10 years ago}", NULL);

	test_object("HEAD@{1 second}", "a65fedf39aefe402d3bb6e24df4d4f5fe4547750");
	test_object("HEAD@{1 second ago}", "a65fedf39aefe402d3bb6e24df4d4f5fe4547750");
	test_object("HEAD@{2 days ago}", "a65fedf39aefe402d3bb6e24df4d4f5fe4547750");

	/*
	 * $ git reflog master --date=iso
	 * a65fedf master@{2012-04-30 09:23:23 -0800}: commit: checking in
	 * be3563a master@{2012-04-30 09:22:43 -0800}: clone: from /Users/ben/src...
	 *
	 * $ git reflog master --date=raw
	 * a65fedf master@{1335806603 -0800}: commit: checking in
	 * be3563a master@{1335806563 -0800}: clone: from /Users/ben/src/libgit2/tests/reso
	 */


	/*
	 * $ git reflog -1 "master@{2012-04-30 17:22:42 +0000}"
	 * warning: Log for 'master' only goes back to Mon, 30 Apr 2012 09:22:43 -0800.
	 */
	test_object("master@{2012-04-30 17:22:42 +0000}", NULL);
	test_object("master@{2012-04-30 09:22:42 -0800}", NULL);

	/*
	 * $ git reflog -1 "master@{2012-04-30 17:22:43 +0000}"
	 * be3563a master@{Mon Apr 30 09:22:43 2012 -0800}: clone: from /Users/ben/src/libg
	 */
	test_object("master@{2012-04-30 17:22:43 +0000}", "be3563ae3f795b2b4353bcce3a527ad0a4f7f644");
	test_object("master@{2012-04-30 09:22:43 -0800}", "be3563ae3f795b2b4353bcce3a527ad0a4f7f644");

	/*
	 * $ git reflog -1 "master@{2012-4-30 09:23:27 -0800}"
	 * a65fedf master@{Mon Apr 30 09:23:23 2012 -0800}: commit: checking in
	 */
	test_object("master@{2012-4-30 09:23:27 -0800}", "a65fedf39aefe402d3bb6e24df4d4f5fe4547750");

	/*
	 * $ git reflog -1 master@{2012-05-03}
	 * a65fedf master@{Mon Apr 30 09:23:23 2012 -0800}: commit: checking in
	 */
	test_object("master@{2012-05-03}", "a65fedf39aefe402d3bb6e24df4d4f5fe4547750");

	/*
	 * $ git reflog -1 "master@{1335806603}"
	 * a65fedf
	 *
	 * $ git reflog -1 "master@{1335806602}"
	 * be3563a
	 */
	test_object("master@{1335806603}", "a65fedf39aefe402d3bb6e24df4d4f5fe4547750");
	test_object("master@{1335806602}", "be3563ae3f795b2b4353bcce3a527ad0a4f7f644");
}

void test_refs_revparse__colon(void)
{
	cl_git_fail(git_revparse_single(&g_obj, g_repo, ":/"));
	cl_git_fail(git_revparse_single(&g_obj, g_repo, ":2:README"));

	test_object(":/not found in any commit", NULL);
	test_object("subtrees:ab/42.txt", NULL);
	test_object("subtrees:ab/4.txt/nope", NULL);
	test_object("subtrees:nope", NULL);
	test_object("test/master^1:branch_file.txt", NULL);

	/* From tags */
	test_object("test:readme.txt", "0266163a49e280c4f5ed1e08facd36a2bd716bcf");
	test_object("tags/test:readme.txt", "0266163a49e280c4f5ed1e08facd36a2bd716bcf");
	test_object("e90810b:readme.txt", "0266163a49e280c4f5ed1e08facd36a2bd716bcf");
	test_object("tags/e90810b:readme.txt", "0266163a49e280c4f5ed1e08facd36a2bd716bcf");

	/* From commits */
	test_object("a65f:branch_file.txt", "3697d64be941a53d4ae8f6a271e4e3fa56b022cc");

	/* From trees */
	test_object("a65f^{tree}:branch_file.txt", "3697d64be941a53d4ae8f6a271e4e3fa56b022cc");
	test_object("944c:branch_file.txt", "3697d64be941a53d4ae8f6a271e4e3fa56b022cc");

	/* Retrieving trees */
	test_object("master:", "944c0f6e4dfa41595e6eb3ceecdb14f50fe18162");
	test_object("subtrees:", "ae90f12eea699729ed24555e40b9fd669da12a12");
	test_object("subtrees:ab", "f1425cef211cc08caa31e7b545ffb232acb098c3");
	test_object("subtrees:ab/", "f1425cef211cc08caa31e7b545ffb232acb098c3");

	/* Retrieving blobs */
	test_object("subtrees:ab/4.txt", "d6c93164c249c8000205dd4ec5cbca1b516d487f");
	test_object("subtrees:ab/de/fgh/1.txt", "1f67fc4386b2d171e0d21be1c447e12660561f9b");
	test_object("master:README", "a8233120f6ad708f843d861ce2b7228ec4e3dec6");
	test_object("master:new.txt", "a71586c1dfe8a71c6cbf6c129f404c5642ff31bd");
	test_object(":/Merge", "a4a7dce85cf63874e984719f4fdd239f5145052f");
	test_object(":/one", "c47800c7266a2be04c571c04d5a6614691ea99bd");
	test_object(":/packed commit t", "41bc8c69075bbdb46c5c6f0566cc8cc5b46e8bd9");
	test_object("test/master^2:branch_file.txt", "45b983be36b73c0788dc9cbcb76cbb80fc7bb057");
	test_object("test/master@{1}:branch_file.txt", "3697d64be941a53d4ae8f6a271e4e3fa56b022cc");
}

void test_refs_revparse__disambiguation(void)
{
	/*
	 * $ git show e90810b
	 * tag e90810b
	 * Tagger: Vicent Marti <tanoku@gmail.com>
	 * Date:   Thu Aug 12 03:59:17 2010 +0200
	 *
	 * This is a very simple tag.
	 *
	 * commit e90810b8df3e80c413d903f631643c716887138d
	 * Author: Vicent Marti <tanoku@gmail.com>
	 * Date:   Thu Aug 5 18:42:20 2010 +0200
	 *
	 *     Test commit 2
	 *
	 * diff --git a/readme.txt b/readme.txt
	 * index 6336846..0266163 100644
	 * --- a/readme.txt
	 * +++ b/readme.txt
	 * @@ -1 +1,2 @@
	 *  Testing a readme.txt
	 * +Now we add a single line here
	 *
	 * $ git show-ref e90810b
	 * 7b4384978d2493e851f9cca7858815fac9b10980 refs/tags/e90810b
	 *
	 */
	test_object("e90810b", "7b4384978d2493e851f9cca7858815fac9b10980");

	/*
	 * $ git show e90810
	 * commit e90810b8df3e80c413d903f631643c716887138d
	 * Author: Vicent Marti <tanoku@gmail.com>
	 * Date:   Thu Aug 5 18:42:20 2010 +0200
	 *
	 *     Test commit 2
	 *
	 * diff --git a/readme.txt b/readme.txt
	 * index 6336846..0266163 100644
	 * --- a/readme.txt
	 * +++ b/readme.txt
	 * @@ -1 +1,2 @@
	 *  Testing a readme.txt
	 * +Now we add a single line here
	 */
	test_object("e90810", "e90810b8df3e80c413d903f631643c716887138d");
}

void test_refs_revparse__a_too_short_objectid_returns_EAMBIGUOUS(void)
{
	int result;
	
	result = git_revparse_single(&g_obj, g_repo, "e90");
	
	cl_assert_equal_i(GIT_EAMBIGUOUS, result);
}
