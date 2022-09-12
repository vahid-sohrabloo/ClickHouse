#!/usr/bin/env python

"""Script to check if PR is mergeable and merge it"""

import argparse
import logging

from datetime import datetime
from typing import Dict, Iterable

from get_robot_token import get_best_robot_token
from github_helper import GitHub, NamedUser, PullRequest
from github.PullRequestReview import PullRequestReview
from pr_info import PRInfo


class Reviews:
    STATES = ["CHANGES_REQUESTED", "APPROVED"]

    def __init__(self, pr: PullRequest):
        """The reviews are proceed in the next logic:
        - if review for an author does not exist, set it
        - the review status can be changed from CHANGES_REQUESTED and APPROVED
            only to either one
        """
        logging.info("Checking the PR for approvals")
        self.pr = pr
        self.reviews = pr.get_reviews()
        # the reviews are ordered by time
        self._review_per_user = {}  # type: Dict[NamedUser, PullRequestReview]
        self.approved_at = datetime.fromtimestamp(0)
        for r in self.reviews:
            user = r.user
            if self._review_per_user.get(user):
                if r.state in self.STATES:
                    self._review_per_user[user] = r
                    if r.state == "APPROVED":
                        self.approved_at = r.submitted_at
                continue
            self._review_per_user[user] = r

    def is_approved(self) -> bool:
        """Checks if the PR is approved, and no changes made after the last approval"""
        if not self.reviews:
            logging.info("There aren't reviews for PR #%s", self.pr.number)
            return False

        statuses = {r.state for r in self._review_per_user.values()}

        if "CHANGES_REQUESTED" in statuses:
            logging.info(
                "The following users requested changes for the PR: %s",
                ", ".join(
                    user.login
                    for user, r in self._review_per_user.items()
                    if r.state == "CHANGES_REQUESTED"
                ),
            )
            return False

        if "APPROVED" in statuses:
            logging.info(
                "The following users approved the PR: %s",
                ", ".join(
                    user.login
                    for user, r in self._review_per_user.items()
                    if r.state == "APPROVED"
                ),
            )
            if self.approved_at < self.pr.head.repo.pushed_at:
                logging.info(
                    "There are changes after approve at %s", self.pr.head.repo.pushed_at
                )
                return False
            return True

        logging.info("The PR #%s is not approved", self.pr.number)
        return False


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


def main():
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(message)s")
    args = parse_args()
    logging.info("Going to process PR #%s in repo %s", args.pr, args.repo)
    gh = GitHub(get_best_robot_token(), per_page=100)
    repo = gh.get_repo(args.repo)
    pr = repo.get_pull(args.pr)
    if pr.is_merged():
        logging.info("The PR #%s is already merged", pr.number)

    if args.check_approved:
        reviews = Reviews(pr)
        if not reviews.is_approved():
            logging.info("We don't merge the PR")
            return

    logging.info("Merging the PR")
    pr.merge()


if __name__ == "__main__":
    main()
