import tweepy
import time
FILE_NAME = 'lastSeenID.txt'

CONSUMER_KEY = 'MVDkHczOFTe6XveqJWmSpc4x0'
CONSUMER_SECRET = 'H7hCpordbt5k69vNHVBwHwM2vmZ7xV8OIUfoExH7Pg8K3VY1X0'
ACCESS_KEY = '1193655897559633920-Svc7Ofqk1hEpIrJWS2ZgVSkm8nFGMk'
ACCESS_SECRET = 'XvA1hmTDln807bjZCw480xXjWJfXM6BPlPwMybVVl8VUS'

#authentication
auth = tweepy.OAuthHandler(CONSUMER_KEY,CONSUMER_SECRET)
auth.set_access_token(ACCESS_KEY, ACCESS_SECRET)
api = tweepy.API(auth)

print('This is my twitter bot')



#takes file name and returns the last seen ID
def retrieveLastSeenID(fileName):
    fileRead = open(fileName, 'r')
    lastSeenID = int(fileRead.read().strip())
    fileRead.close()
    return lastSeenID
#takes new last seen ID and fileName and then write new last seen ID to text file
def storeLastSeenID(lastSeenID, fileName):
    fileWrite = open(fileName, 'w')
    fileWrite.write(str(lastSeenID))
    fileWrite.close()
    return

def replyToTweets():
    print('Retrieving and replying to tweets...')
    #DEV note: use 1193669727366893568 (first tweet id) for testing
    lastSeenID = retrieveLastSeenID(FILE_NAME)
    mentions = api.mentions_timeline(lastSeenID, tweet_mode = 'extended')

    for mention in reversed(mentions):
        print(str(mention.id) + ' - ' + mention.full_text)
        lastSeenID = mention.id
        storeLastSeenID(lastSeenID, FILE_NAME)
        if '#helloworld' in mention.full_text.lower():
            print('found #helloworld')
            print('responding back...')
            api.update_status('@' + mention.user.screen_name + ' #HelloWorld back to you!', mention.id)

while True:
    replyToTweets()
    time.sleep(15)