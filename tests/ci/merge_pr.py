#!/usr/bin/env python

"""Script to check if PR is mergeable and merge it"""

import argparse
import logging

from get_robot_token import get_best_robot_token
from github_helper import GitHub, PullRequest
from pr_info import PRInfo


def parse_args() -> argparse.Namespace:
    pr_info = PRInfo()
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        description="Script to download binary artifacts from S3. Downloaded artifacts "
        "are renamed to clickhouse-{static_binary_name}",
    )
    parser.add_argument(
        "--check-approved",
        action="store_true",
        help="if set, checks that the PR is approved and no changes required",
    )
    parser.add_argument(
        "--repo",
        default=pr_info.repo_full_name,
        help="PR number to check",
    )
    parser.add_argument(
        "--pr",
        type=int,
        default=pr_info.number,
        help="PR number to check",
    )
    args = parser.parse_args()
    return args


def is_approved(pr: PullRequest) -> bool:
    logging.info("Checking the PR for approvals")
    reviews = pr.get_reviews()
    if not reviews.totalCount:
        logging.info("There aren't reviews for PR #%s", pr.number)
        return False
    # We get reviews in a chronological order, so we'll have the only latest
    # review per user
    review_per_user = {r.user: r.state for r in reviews}
    if "CHANGES_REQUESTED" in review_per_user.values():
        logging.info(
            "The following users requested changes for the PR: %s",
            ", ".join(
                user.login
                for user, state in review_per_user.items()
                if state == "CHANGES_REQUESTED"
            ),
        )
        return False

    if "APPROVED" in review_per_user.values():
        logging.info(
            "The following users approved the PR: %s",
            ", ".join(
                user.login
                for user, state in review_per_user.items()
                if state == "APPROVED"
            ),
        )
        return True
    logging.info("The PR is not approved")
    return False


def main():
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(message)s")
    args = parse_args()
    logging.info("Going to process PR #%s in repo %s", args.pr, args.repo)
    gh = GitHub(get_best_robot_token(), per_page=100)
    repo = gh.get_repo(args.repo)
    pr = repo.get_pull(args.pr)
    if pr.is_merged():
        logging.info("The PR #%s is already merged", pr.number)
    if args.check_approved and not is_approved(pr):
        logging.info("We don't merge the PR")
        return
    logging.info("Merging the PR")
    pr.merge()


if __name__ == "__main__":
    main()
